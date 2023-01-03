#include <iostream>

#include "project_run.hpp"

using namespace os2cx;

// Copy this code from gui_mode_result.cpp and port it over to work in non-GUI code
enum class SubVariable {
	ScalarValue,
        VectorMagnitude, VectorX, VectorY, VectorZ,
        ComplexVectorMagnitude,
        MatrixVonMisesStress,
        MatrixXX, MatrixYY, MatrixZZ, MatrixXY, MatrixYZ, MatrixZX
};

double subvariable_value(
    const Results::Dataset &dataset,
    SubVariable subvar,
    NodeId node_id
) {
    switch (subvar) {
    case SubVariable::ScalarValue:
        return (*dataset.node_scalar)[node_id];
    case SubVariable::VectorMagnitude:
        return (*dataset.node_vector)[node_id].magnitude();
    case SubVariable::VectorX:
        return (*dataset.node_vector)[node_id].x;
    case SubVariable::VectorY:
        return (*dataset.node_vector)[node_id].y;
    case SubVariable::VectorZ:
        return (*dataset.node_vector)[node_id].z;
    case SubVariable::ComplexVectorMagnitude:
        return (*dataset.node_complex_vector)[node_id].magnitude();
    case SubVariable::MatrixVonMisesStress:
        return von_mises_stress((*dataset.node_matrix)[node_id]);
    case SubVariable::MatrixXX:
        return (*dataset.node_matrix)[node_id].cols[0].x;
    case SubVariable::MatrixYY:
        return (*dataset.node_matrix)[node_id].cols[1].y;
    case SubVariable::MatrixZZ:
        return (*dataset.node_matrix)[node_id].cols[2].z;
    case SubVariable::MatrixXY:
        return (*dataset.node_matrix)[node_id].cols[0].y;
    case SubVariable::MatrixYZ:
        return (*dataset.node_matrix)[node_id].cols[1].z;
    case SubVariable::MatrixZX:
        return (*dataset.node_matrix)[node_id].cols[2].x;
    default: assert(false);
    }
}


int main(int argc, char *argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: os2cx path/to/file.scad" << std::endl;
        return 1;
    }
    std::string scad_path = argv[1];

    os2cx::Project _project(scad_path);
    os2cx::ProjectRunCallbacks callbacks;
    os2cx::project_run(&_project, &callbacks);
    std::cout << "Looping through results for measurements" << std::endl;
    // std::shared_ptr<const os2cx::Project> project = std::shared_ptr<const os2cx::Project>(&_project);
    const os2cx::Project* project = &_project;
    // Only handle one set of results, not sure what multiple would mean and probably breaks result_ptr->steps[0] below
    assert(project->results->results.size() == 1);
    for (const os2cx::Results::Result &result : project->results->results) {
      const os2cx::Results::Result *result_ptr = &result;

      std::cout << "Number of measurements: " << project->measure_objects.size() << std::endl;
      // Copy this code from refresh_measurements() in gui_mode_result.cpp and port it over to work in non-GUI code
      const Results::Result::Step &step = result_ptr->steps[0]; // Was step_index in gui_mode_result.cpp, now hard coded to 0
      for (const auto &measure_pair : project->measure_objects) {
	std::string item_name = measure_pair.first;
        double max_datum;
        auto dataset_it = step.datasets.find(measure_pair.second.dataset);
        if (dataset_it == step.datasets.end()) {
            max_datum = NAN;
        } else {
            max_datum = 0;
            const Results::Dataset &dataset = dataset_it->second;
            SubVariable measure_subvariable;
            if (dataset.node_scalar) {
                measure_subvariable = SubVariable::ScalarValue;
            } else if (dataset.node_vector) {
                measure_subvariable = SubVariable::VectorMagnitude;
            } else if (dataset.node_complex_vector) {
                measure_subvariable = SubVariable::ComplexVectorMagnitude;
            } else if (dataset.node_matrix) {
                measure_subvariable = SubVariable::MatrixVonMisesStress;
            } else {
                assert(false);
            }

            std::string subject = measure_pair.second.subject;
            std::shared_ptr<const NodeSet> node_set;
            const Project::VolumeObject *volume;
            const Project::SurfaceObject *surface;
            const Project::NodeObject *node;
            if ((volume = project->find_volume_object(subject))) {
                node_set = volume->node_set;
            } else if ((surface = project->find_surface_object(subject))) {
                node_set = surface->node_set;
            } else if ((node = project->find_node_object(subject))) {
                node_set.reset(new NodeSet(
                    compute_node_set_singleton(node->node_id)));
            }

            for (NodeId node_id : node_set->nodes) {
                double datum = subvariable_value(
                    dataset,
                    measure_subvariable,
                    node_id);
                if (isnan(datum)) {
                    max_datum = NAN;
                    break;
                } else {
                    max_datum = std::max(datum, max_datum);
                }
            }
        }

        double item_value;
	std::string item_units;
        if (isnan(max_datum)) {
	    item_value = 0.0;
	    item_units = "N/A";
        } else {
            Unit unit = project->unit_system.suggest_unit(
                guess_unit_type_for_dataset(measure_pair.second.dataset),
                max_datum);
            WithUnit<double> max_datum_2 =
                project->unit_system.system_to_unit(unit, max_datum);
	    item_value = max_datum_2.value_in_unit;
	    item_units = unit.name;
        }
	std::cout << "Measurement name=" << item_name.c_str() << " value=" << item_value << " unit=" << item_units.c_str() << std::endl;
      }
    }

    return 0;
}
