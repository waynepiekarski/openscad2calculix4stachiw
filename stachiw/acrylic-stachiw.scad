// All units are in meters

include <../openscad2calculix.scad>

// Material properties for acrylic
// Poisson's ratio: https://www.matweb.com/search/DataSheet.aspx?MatGUID=632572aeef2a4224b5ac8fbd4f1b6f77
// Young's modulus: https://www.engineeringtoolbox.com/young-modulus-d_417.html
os2cx_material_elastic_simple(
    "acrylic",
    youngs_modulus=[2.25, "GPa"], /* 1.20 - 3.38 matweb, 1.4 - 3.1 engtoolbox */
    poissons_ratio=0.4, /* 0.37-0.43 */
    density=[1185, "kg/m^3"]); /* 1.185 g/cc */

resolution = 100;

// Stachiw experiments from Figure 7.12 are specified in inches and psi, tdi is t/Di (thickness divided by diameter)
Di = is_undef(inject_diameter) ? 1.5 : inject_diameter;
tdi = is_undef(inject_tdi) ? 0.559 : inject_tdi;
t = tdi * Di;
pressure_psi = is_undef(inject_psi) ? 20000 : inject_psi;

function get_meters_from_inches(inch) = 0.0254 * inch;
Di_meters = get_meters_from_inches(Di);
t_meters = get_meters_from_inches(t);
echo("Di(in)", Di, "t(in)", t, "Di(m)", Di_meters, "t(m)", t_meters, "Di(mm)", Di_meters*1000, "t(mm)", t_meters*1000, "t/Di", tdi);
tolerance = 0.001;
echo("Edge and support tolerance(mm)", tolerance*1000);

// Mesh object in OS2CX
os2cx_mesh("test_disc", material="acrylic") {
    cylinder(d=Di_meters, h=t_meters, $fn=resolution, center=true);
}

// Support the acrylic disc, which is important in getting the right results.
// With supports on the entire cylinder sides, the deflection is micrometers.
// With a thin ring around the bottom edge, the deflection is a more reasonable 1.2mm
// At 6000 psi we are expecting about 0.025"=0.635mm from figure 7.12 but the graph is hard to read
// At 20000 psi fig 7.12 should be 0.15"=3.81mm, the deflection calculated is 4.6mm
os2cx_select_volume("supported_edge") {
    translate([0,0,-t_meters/2])
    difference() {
        cylinder(d=Di_meters+tolerance, h=1*tolerance, $fn=resolution, center=true);
        cylinder(d=Di_meters-tolerance, h=2*tolerance, $fn=resolution, center=true);
    }
}

os2cx_select_surface("load_surface", [0, 0, +1], 45) /* tolerance angle = 45 degrees */ {
    translate([0,0,t_meters/2])
        cylinder(d=Di_meters-tolerance, h=2*tolerance, $fn=resolution, center=true);
}


// Apply pressure across entire load surface in psi
// Convert from psi into Pa since os2cx doesn't support psi or lb/in^2 directly
function get_pascal_from_psi(psi) = 6894.76 * psi;
pressure_Pa = get_pascal_from_psi(pressure_psi);
echo("pressure(psi)", pressure_psi, "pressure(Pa)", pressure_Pa, "pressure(MPa)", pressure_Pa/1000000.0);
os2cx_load_surface(
    "applied_weight",
    "load_surface",
    force_per_area=[[0, 0, -1 * pressure_Pa], "Pa"]);

os2cx_analysis_static_simple(
    fixed="supported_edge",
    load="applied_weight",
    length_unit="m"
);

// Report the maximum deflection of the loaded part
os2cx_measure(
    "max_deflection_result",
    "load_surface",
    "DISP");
