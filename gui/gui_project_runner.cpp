#include "gui_project_runner.hpp"

namespace os2cx {

GuiProjectRunnerWorkerThread::GuiProjectRunnerWorkerThread(
    QObject *parent,
    const Project &original_project) :
    QThread(parent)
{
    project_on_worker_thread.reset(new Project(original_project));
    checkpoint_active = false;
}

void GuiProjectRunnerWorkerThread::run() {
    mutex.lock();
    try {
        project_run(project_on_worker_thread.get(), this);
        project_run_checkpoint();
    } catch (const ProjectInterruptedException &) {
        /* Do nothing. */
    }
    mutex.unlock();
}

/* project_run() calls project_run_log() on the worker thread. */
void GuiProjectRunnerWorkerThread::project_run_log(const std::string &msg) {
    emit log_signal(QString::fromStdString(msg));
}


/* project_run() calls project_run_checkpoint() on the worker thread */
void GuiProjectRunnerWorkerThread::project_run_checkpoint() {
    checkpoint_active = true;
    emit checkpoint_signal();

    /* The QWaitCondition documentation doesn't explicitly state whether or not
    QWaitCondition is susceptible to spurious wakeups; better safe than sorry.
    */
    while (checkpoint_active) {
        wait_condition.wait(&mutex);
    }
    assert(!checkpoint_active);

    if (isInterruptionRequested()) {
        throw ProjectInterruptedException();
    }
}

GuiProjectRunner::GuiProjectRunner(
        QObject *parent,
        const std::string &scad_path) :
    QObject(parent),
    project_on_application_thread(new Project(scad_path)),
    interrupted(false),
    last_emitted_status(Status::Running)
{
    worker_thread.reset(new GuiProjectRunnerWorkerThread(
        this,
        *project_on_application_thread
    ));
    connect(
        worker_thread.get(), &GuiProjectRunnerWorkerThread::log_signal,
        this, &GuiProjectRunner::log_slot);
    connect(
        worker_thread.get(), &GuiProjectRunnerWorkerThread::checkpoint_signal,
        this, &GuiProjectRunner::checkpoint_slot);
    connect(
        worker_thread.get(), &QThread::finished,
        this, &GuiProjectRunner::maybe_emit_status_changed);
    worker_thread->start();
}

GuiProjectRunner::Status GuiProjectRunner::status() const {
    if (interrupted) {
        if (worker_thread->isFinished()) {
            return Status::Interrupted;
        } else {
            return Status::Interrupting;
        }
    } else {
        /* Don't rely on worker_thread->isFinished() because it might change to
        true before we receive the final update, and it would be weird if status
        was Status::Done while the project progress wasn't yet
        Progress::AllDone. To avoid confusion, check project progress directly.
        */
        if (project_on_application_thread->errored) {
            return Status::Errored;
        } else if (project_on_application_thread->progress ==
                Project::Progress::AllDone) {
            return Status::Done;
        } else {
            return Status::Running;
        }
    }
}

void GuiProjectRunner::interrupt() {
    if (!interrupted) {
        worker_thread->requestInterruption();
        interrupted = true;
        log_slot(tr("Interrupting calculation..."));
        maybe_emit_status_changed();
    }
}

void GuiProjectRunner::maybe_emit_status_changed() {
    Status new_status = status();
    if (new_status != last_emitted_status) {
        last_emitted_status = new_status;
        if (new_status == Status::Interrupted) {
            log_slot(tr("Interrupted calculation."));
        }
        emit status_changed(new_status);
    }
}

void GuiProjectRunner::log_slot(const QString &msg) {
    logs.push_back(msg);
    emit project_logged();
}

void GuiProjectRunner::checkpoint_slot() {
    {
        QMutexLocker mutex_locker(&worker_thread->mutex);
        *project_on_application_thread =
            *worker_thread->project_on_worker_thread;
        worker_thread->checkpoint_active = false;
        worker_thread->wait_condition.wakeAll();
    }

    emit project_updated();
    maybe_emit_status_changed();
}

} /* namespace os2cx */
