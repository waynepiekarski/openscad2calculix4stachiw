#include "openscad_extract.hpp"

#include <iostream>
#include <sstream>

#include "openscad_run.hpp"

namespace os2cx {

std::unique_ptr<OpenscadRun> call_openscad(
    Project *project,
    const std::string &geometry_file_name,
    std::vector<OpenscadValue> &&mode
) {
    std::map<std::string, OpenscadValue> defines;
    defines.insert(std::make_pair(
        "__openscad2calculix_mode",
        OpenscadValue(std::move(mode))
    ));
    std::unique_ptr<OpenscadRun> run(new OpenscadRun(
        project->scad_path,
        project->temp_dir + "/" + geometry_file_name + ".off",
        defines
    ));
    run->wait();
    return std::move(run);
}

void check_arg_count(
    const std::vector<OpenscadValue> &args, int expect, const char *what
) {
    if (static_cast<int>(args.size()) != expect) {
        throw BadEchoError("wrong number of arguments to " + std::string(what));
    }
}

std::string check_string(const OpenscadValue &value) {
    if (value.type != OpenscadValue::Type::String) {
        throw BadEchoError("expected string");
    }
    return value.string_value;
}

double check_number(const OpenscadValue &value) {
    if (value.type != OpenscadValue::Type::Number) {
        throw BadEchoError("expected number");
    }
    return value.number_value;
}

std::string check_name_new(
    const OpenscadValue &value,
    const std::string &new_object_type,
    Project *project
) {
    std::string new_name = check_string(value);

    // TODO: Check character set

    std::string existing_object_type;
    if (project->mesh_objects.count(new_name)) {
        existing_object_type = "mesh";
    } else if (project->select_volume_objects.count(new_name)) {
        existing_object_type = "volume";
    } else if (project->select_surface_objects.count(new_name)) {
        existing_object_type = "surface";
    } else if (project->load_objects.count(new_name)) {
        existing_object_type = "load";
    }
    if (!existing_object_type.empty()) {
        throw UsageError("Can't declare a new " + new_object_type + " named '" +
            new_name + "' because there already exists a " +
            existing_object_type + " named '" + new_name + "'.");
    }

    return new_name;
}

template<class Inner, class InnerConverter>
std::vector<Inner> check_vector(
    const OpenscadValue &value,
    const InnerConverter &inner_converter
) {
    std::vector<Inner> result;
    if (value.type != OpenscadValue::Type::Vector) {
        throw BadEchoError("expected vector");
    }
    for (const OpenscadValue &inner_value : value.vector_value) {
        result.push_back(inner_converter(inner_value));
    }
    return result;
}

Vector check_vector_3(const OpenscadValue &value) {
    std::vector<double> parts = check_vector<double>(value, &check_number);
    if (parts.size() != 3) {
        throw BadEchoError("expected vector to have 3 elements");
    }
    return Vector(parts[0], parts[1], parts[2]);
}

void do_analysis_directive(
    Project *project,
    const std::vector<OpenscadValue> &args
) {
    check_arg_count(args, 2, "analysis");

    if (!project->calculix_deck.empty()) {
        throw UsageError("Can't have multiple os2cx_analysis_...() directives "
            "in the same file.");
    }

    std::vector<std::string> deck =
        check_vector<std::string>(args[0], &check_string);
    if (deck.empty()) {
        throw BadEchoError("empty analysis_directive");
    }
    project->calculix_deck = std::move(deck);

    std::vector<std::string> units =
        check_vector<std::string>(args[1], &check_string);
    if (units.size() != 3) {
        throw BadEchoError("bad unit system in analysis_directive");
    }
    try {
        project->unit_system = UnitSystem(units[0], units[1], units[2]);
    } catch (const UnitParseError &e) {
        throw UsageError("Invalid unit system in analysis directive: " +
            std::string(e.what()));
    }
}

void do_mesh_directive(
    Project *project,
    const std::vector<OpenscadValue> &args
) {
    check_arg_count(args, 1, "mesh");

    Project::MeshObjectName name = check_name_new(args[0], "mesh", project);

    project->mesh_objects.insert(std::make_pair(
        name,
        Project::MeshObject()
    ));
}

void do_select_volume_directive(
    Project *project,
    const std::vector<OpenscadValue> &args
) {
    check_arg_count(args, 1, "select_volume");

    Project::SelectVolumeObjectName name =
        check_name_new(args[0], "volume", project);

    Project::SelectVolumeObject object;

    /* If there are too many select_volume directives, bit_index will be greater
    than or equal to Plc3::num_bits; we'll check this later. */
    object.bit_index = project->next_bit_index++;

    project->select_volume_objects.insert(std::make_pair(name, object));
}

void do_select_surface_directive(
    Project *project,
    const std::vector<OpenscadValue> &args
) {
    check_arg_count(args, 3, "select_surface");

    Project::SelectSurfaceObjectName name =
        check_name_new(args[0], "surface", project);

    Project::SelectSurfaceObject object;

    /* If there are too many select_* directives, bit_index will be greater than
    or equal to Plc3::num_bits; we'll check this later. */
    object.bit_index = project->next_bit_index++;

    object.direction_vector = check_vector_3(args[1]);

    object.direction_angle_tolerance = check_number(args[2]);

    project->select_surface_objects.insert(std::make_pair(name, object));
}

void do_load_volume_directive(
    Project *project,
    const std::vector<OpenscadValue> &args
) {
    check_arg_count(args, 3, "load_volume");

    Project::LoadObjectName name = check_name_new(args[0], "load", project);

    Project::VolumeObjectName volume = check_string(args[1]);
    if (project->find_volume_object(volume) == nullptr) {
        throw UsageError("Load '" + name + "' refers to volume '" + volume +
            "', which does not exist (yet).");
    }
    project->load_objects[name].volume = volume;

    Vector force_density(0, 0, check_number(args[2]));
    project->load_objects[name].force_density = force_density;
}

void openscad_extract_inventory(Project *project) {
    std::unique_ptr<OpenscadRun> run = call_openscad(
        project,
        "inventory",
        { OpenscadValue("inventory") });

    for (const std::string &warning : run->warnings) {
        std::cerr << "openscad: WARNING: " << warning << std::endl;
    }

    if (run->geometry != nullptr) {
        throw UsageError("All toplevel geometry must be wrapped in a directive "
            "such as 'os2cx_mesh()'; for example, 'os2cx_mesh() cube(...)' "
            "instead of 'cube(...)'. Your file has some toplevel geometry that "
            "isn't properly wrapped.");
    }

    for (const std::vector<OpenscadValue> &echo : run->echos) {
        if (echo.size() == 0 ||
            echo[0] != OpenscadValue("__openscad2calculix")) {
            continue;
        }
        try {
            if (echo.size() == 1 ||
                    echo[1].type != OpenscadValue::Type::String) {
                throw BadEchoError("missing or malformed subdirective");
            }
            std::vector<OpenscadValue> args(echo.begin() + 2, echo.end());
            if (echo[1].string_value == "analysis_directive") {
                do_analysis_directive(project, args);
            } else if (echo[1].string_value == "mesh_directive") {
                do_mesh_directive(project, args);
            } else if (echo[1].string_value == "select_volume_directive") {
                do_select_volume_directive(project, args);
            } else if (echo[1].string_value == "select_surface_directive") {
                do_select_surface_directive(project, args);
            } else if (echo[1].string_value == "load_volume_directive") {
                do_load_volume_directive(project, args);
            } else {
                throw BadEchoError(
                    "unknown directive: " + echo[1].string_value);
            }
        } catch (const BadEchoError &error) {
            std::stringstream msg;
            msg << "in line";
            for (const OpenscadValue &value : echo) {
                msg << ' ' << value;
            }
            msg << ": " << error.what();
            throw BadEchoError(msg.str());
        }
    }

    if (project->next_bit_index > Plc3::num_bits) {
        std::stringstream msg;
        msg << "There are too many os2cx_select_volume() and/or "
            << "os2cx_select_surface() directives. There are "
            << (project->select_volume_objects.size() +
                project->select_surface_objects.size())
            << ", but the limit is " << Plc3::num_bits - 1 << ".";
        throw UsageError(msg.str());
    }

    if (project->calculix_deck.empty()) {
        throw UsageError("Please specify an os2cx_analysis_...() directive.");
    }
}

std::unique_ptr<Poly3> openscad_extract_poly3(
    Project *project,
    const std::string &object_type,
    const std::string &name
) {
    std::unique_ptr<OpenscadRun> run = call_openscad(
        project,
        name,
        { OpenscadValue(object_type), OpenscadValue(name) });
    if (!run->geometry) {
        throw UsageError("Empty " + object_type + " '" + name + "'.");
    }
    return std::move(run->geometry);
}

} /* namespace os2cx */
