#!/usr/bin/env python3
"""Parametric servo-powered cap mount for the Axiometa AX22-0034 display."""

from __future__ import annotations

import argparse
import json
import math
from dataclasses import asdict, dataclass
from pathlib import Path

import cadquery as cq


@dataclass(frozen=True)
class Params:
    # Brim clamp. The wedge accommodates roughly 3-5.5 mm brims.
    clamp_depth: float = 40.0
    clamp_width: float = 30.0
    clamp_top: float = 3.0
    clamp_drop: float = 9.0
    wedge_length: float = 35.0
    wedge_width: float = 24.8
    wedge_front_gap: float = 5.5
    wedge_rear_gap: float = 3.4

    # MG90S envelope. Clones vary, so the body aperture has 0.7 mm clearance.
    servo_body_length: float = 22.8
    servo_body_depth: float = 28.5
    servo_body_thickness: float = 12.2
    servo_body_clearance: float = 0.7
    servo_shaft_from_front: float = 5.8
    servo_mount_spacing: float = 28.0
    servo_mount_pilot: float = 1.7
    servo_plate_thickness: float = 4.2
    servo_pivot_x: float = -5.0
    servo_pivot_y: float = -42.0
    servo_pivot_z: float = 14.0

    # Moving arm. The screen face is 70 mm beyond the brim edge.
    screen_face_reach: float = 70.0
    screen_drop: float = 26.0
    screen_center_y: float = -28.0
    arm_width: float = 10.0
    arm_thickness: float = 4.0
    horn_root_radius: float = 11.0
    park_angle: float = 90.0

    # AX22-0034 documented envelope.
    pcb_width: float = 29.0
    pcb_height: float = 22.0
    glass_width: float = 27.9
    glass_height: float = 13.5
    board_clearance: float = 0.4
    cradle_wall: float = 1.8
    cradle_depth: float = 10.2
    bezel_thickness: float = 2.2


def box_at(x: float, y: float, z: float, dx: float, dy: float, dz: float) -> cq.Workplane:
    return (
        cq.Workplane("XY")
        .box(dx, dy, dz)
        .translate((x + dx / 2, y + dy / 2, z + dz / 2))
    )


def cylinder_y(radius: float, length: float, x: float, y: float, z: float) -> cq.Workplane:
    return (
        cq.Workplane("XZ")
        .center(x, z)
        .circle(radius)
        .extrude(length / 2, both=True)
        .translate((0, y, 0))
    )


def plate_xz(points: list[tuple[float, float]], thickness: float, y: float) -> cq.Workplane:
    return (
        cq.Workplane("XZ")
        .polyline(points)
        .close()
        .extrude(thickness / 2, both=True)
        .translate((0, y, 0))
    )


def ground(shape: cq.Workplane) -> cq.Workplane:
    bbox = shape.val().BoundingBox()
    return shape.translate((0, 0, -bbox.zmin))


def make_base(p: Params) -> tuple[cq.Workplane, tuple[float, float, float]]:
    half_width = p.clamp_width / 2
    wall = 2.2
    ledge = 2.3

    top = box_at(0, -half_width, 0, p.clamp_depth, p.clamp_width, p.clamp_top)
    right_wall = box_at(0, -half_width, -p.clamp_drop, p.clamp_depth, wall, p.clamp_drop)
    left_wall = box_at(
        0,
        half_width - wall,
        -p.clamp_drop,
        p.clamp_depth,
        wall,
        p.clamp_drop,
    )
    back_stop = box_at(
        p.clamp_depth - 3,
        -half_width,
        -p.clamp_drop,
        3,
        p.clamp_width,
        p.clamp_drop,
    )
    right_ledge = box_at(
        0,
        -half_width + wall,
        -p.clamp_drop,
        p.clamp_depth - 3,
        ledge,
        1.3,
    )
    left_ledge = box_at(
        0,
        half_width - wall - ledge,
        -p.clamp_drop,
        p.clamp_depth - 3,
        ledge,
        1.3,
    )
    base = top.union(right_wall).union(left_wall).union(back_stop)
    base = base.union(right_ledge).union(left_ledge)

    # The servo lies on its side so the output axis raises the display in XZ.
    pivot = (p.servo_pivot_x, p.servo_pivot_y, p.servo_pivot_z)
    body_center_x = pivot[0] + p.servo_body_length / 2 - p.servo_shaft_from_front
    plate_y = pivot[1] + 5.0
    plate_width = p.servo_mount_spacing + 8.0
    plate_height = p.servo_body_thickness + 7.0
    plate_x = body_center_x - plate_width / 2
    plate_z = pivot[2] - plate_height / 2

    servo_plate = box_at(
        plate_x,
        plate_y,
        plate_z,
        plate_width,
        p.servo_plate_thickness,
        plate_height,
    )
    aperture = box_at(
        body_center_x - (p.servo_body_length + p.servo_body_clearance) / 2,
        plate_y - 1.0,
        pivot[2] - (p.servo_body_thickness + p.servo_body_clearance) / 2,
        p.servo_body_length + p.servo_body_clearance,
        p.servo_plate_thickness + 2.0,
        p.servo_body_thickness + p.servo_body_clearance,
    )
    servo_plate = servo_plate.cut(aperture)

    # A low shelf and two ribs connect the servo frame to the rigid clamp shell.
    bridge_y = plate_y + p.servo_plate_thickness
    bridge_depth = -13.0 - bridge_y
    lower_shelf = box_at(plate_x, bridge_y, 1.0, plate_width, bridge_depth, 3.0)
    front_rib = box_at(plate_x, bridge_y, 1.0, 5.0, bridge_depth, plate_height - 0.5)
    rear_rib = box_at(
        plate_x + plate_width - 5.0,
        bridge_y,
        1.0,
        5.0,
        bridge_depth,
        plate_height - 0.5,
    )
    base = base.union(servo_plate).union(lower_shelf).union(front_rib).union(rear_rib)

    # The supplied servo mounting screws self-tap into these pilots.
    for x in (
        body_center_x - p.servo_mount_spacing / 2,
        body_center_x + p.servo_mount_spacing / 2,
    ):
        base = base.cut(cylinder_y(p.servo_mount_pilot / 2, 9.0, x, plate_y + 1.5, pivot[2]))

    # Grooves align protective tape on the upper jaw.
    for x in (8.0, 28.0):
        base = base.cut(box_at(x, -10.0, -0.45, 6.0, 20.0, 0.5))

    return base, pivot


def make_wedge(p: Params) -> cq.Workplane:
    bottom = -p.clamp_drop + 1.3
    front_top = -p.wedge_front_gap
    rear_top = -p.wedge_rear_gap
    profile = [
        (0.0, bottom),
        (p.wedge_length, bottom),
        (p.wedge_length, rear_top),
        (0.0, front_top),
    ]
    wedge = plate_xz(profile, p.wedge_width, 0)
    tab = box_at(-8.0, -p.wedge_width / 2 - 1.2, bottom, 8.5, p.wedge_width + 2.4, 2.6)
    wedge = wedge.union(tab)

    for x in (8.0, 17.0, 26.0):
        z = front_top + (rear_top - front_top) * (x / p.wedge_length)
        wedge = wedge.union(box_at(x - 0.55, -9.0, z - 0.1, 1.1, 18.0, 0.45))
    return wedge


def make_servo_fit_gauge(p: Params) -> cq.Workplane:
    pivot = (p.servo_pivot_x, 0.0, p.servo_pivot_z)
    body_center_x = pivot[0] + p.servo_body_length / 2 - p.servo_shaft_from_front
    width = p.servo_mount_spacing + 8.0
    height = p.servo_body_thickness + 7.0
    gauge = box_at(
        body_center_x - width / 2,
        -1.2,
        pivot[2] - height / 2,
        width,
        2.4,
        height,
    )
    gauge = gauge.cut(
        box_at(
            body_center_x - (p.servo_body_length + p.servo_body_clearance) / 2,
            -2.0,
            pivot[2] - (p.servo_body_thickness + p.servo_body_clearance) / 2,
            p.servo_body_length + p.servo_body_clearance,
            4.0,
            p.servo_body_thickness + p.servo_body_clearance,
        )
    )
    for x in (
        body_center_x - p.servo_mount_spacing / 2,
        body_center_x + p.servo_mount_spacing / 2,
    ):
        gauge = gauge.cut(cylinder_y(p.servo_mount_pilot / 2, 4.0, x, 0.0, pivot[2]))
    return gauge


def make_arm(
    p: Params, pivot: tuple[float, float, float]
) -> tuple[cq.Workplane, tuple[float, float, float]]:
    px, py, pz = pivot
    arm_y = py - 5.8
    screen_x = -p.screen_face_reach - 4.0
    screen_z = pz - p.screen_drop
    end = (screen_x, screen_z)

    dx = end[0] - px
    dz = end[1] - pz
    length = math.hypot(dx, dz)
    nx = -dz / length
    nz = dx / length
    half = p.arm_width / 2
    body_points = [
        (px + nx * half, pz + nz * half),
        (end[0] + nx * half, end[1] + nz * half),
        (end[0] - nx * half, end[1] - nz * half),
        (px - nx * half, pz - nz * half),
    ]

    body = plate_xz(body_points, p.arm_thickness, arm_y)
    root = cylinder_y(p.horn_root_radius, p.arm_thickness, px, arm_y, pz)
    tab_points = [
        (end[0] - 5.0, end[1] - 14.0),
        (end[0] + 5.0, end[1] - 14.0),
        (end[0] + 5.0, end[1] + 14.0),
        (end[0] - 5.0, end[1] + 14.0),
    ]
    arm = body.union(root).union(plate_xz(tab_points, p.arm_thickness, arm_y))

    # The center screw reaches the spline through the arm and supplied horn.
    arm = arm.cut(cylinder_y(3.7, 8.0, px, arm_y, pz))

    # Cross-shaped slots fit the common round, cross, or double MG90S horns.
    slot_inner = 4.5
    slot_length = 5.5
    slot_width = 1.9
    for sign in (-1.0, 1.0):
        arm = arm.cut(
            box_at(
                px + sign * slot_inner - (slot_length if sign < 0 else 0),
                arm_y - 4.0,
                pz - slot_width / 2,
                slot_length,
                8.0,
                slot_width,
            )
        )
        arm = arm.cut(
            box_at(
                px - slot_width / 2,
                arm_y - 4.0,
                pz + sign * slot_inner - (slot_length if sign < 0 else 0),
                slot_width,
                8.0,
                slot_length,
            )
        )

    for z in (end[1] - 6.0, end[1] + 6.0):
        arm = arm.cut(cylinder_y(1.75, 8.0, end[0], arm_y, z))

    # Cable ties keep the display lead against the outside of the arm.
    for t in (0.35, 0.68):
        x = px + dx * t
        z = pz + dz * t
        arm = arm.cut(cylinder_y(2.0, 8.0, x, arm_y, z))

    return arm, (end[0], arm_y, end[1])


def make_cradle(p: Params, arm_end: tuple[float, float, float]) -> cq.Workplane:
    mount_x, _, center_z = arm_end
    outer_width = p.pcb_width + 4.0
    outer_height = p.pcb_height + 4.0
    center_y = p.screen_center_y
    y0 = center_y - outer_width / 2
    z0 = center_z - outer_height / 2
    front_x = -p.screen_face_reach
    back_x = front_x - p.cradle_depth
    inner_width = p.pcb_width + p.board_clearance
    inner_height = p.pcb_height + p.board_clearance

    bezel = box_at(
        front_x,
        y0,
        z0,
        p.bezel_thickness,
        outer_width,
        outer_height,
    )
    window = box_at(
        front_x - 1,
        center_y - (p.glass_width + 0.7) / 2,
        center_z - (p.glass_height + 0.7) / 2,
        p.bezel_thickness + 2,
        p.glass_width + 0.7,
        p.glass_height + 0.7,
    )
    cradle = bezel.cut(window)

    depth = p.cradle_depth + p.bezel_thickness
    wall = p.cradle_wall
    right_wall = box_at(back_x, y0, z0, depth, wall, outer_height)
    left_wall = box_at(back_x, y0 + outer_width - wall, z0, depth, wall, outer_height)
    bottom_wall = box_at(back_x, y0, z0, depth, outer_width, wall)
    top_wall = box_at(back_x, y0, z0 + outer_height - wall, depth, outer_width, wall)

    connector_notch = box_at(
        back_x - 0.5,
        y0 - 0.5,
        center_z - 7.0,
        p.cradle_depth * 0.62,
        wall + 1.2,
        14.0,
    )
    right_wall = right_wall.cut(connector_notch)
    cradle = cradle.union(right_wall).union(left_wall).union(bottom_wall).union(top_wall)

    cavity_y0 = center_y - inner_width / 2
    cavity_z0 = center_z - inner_height / 2
    for y in (cavity_y0 + 4.0, cavity_y0 + inner_width - 9.0):
        cradle = cradle.union(box_at(back_x, y, cavity_z0, 1.5, 5.0, 0.7))
        cradle = cradle.union(
            box_at(back_x, y, cavity_z0 + inner_height - 0.7, 1.5, 5.0, 0.7)
        )

    # The arm overlaps these bosses and is retained by two M3 screws.
    outer_y = y0 - 4.0
    for z in (center_z - 6.0, center_z + 6.0):
        cradle = cradle.union(cylinder_y(4.0, 8.0, mount_x, outer_y, z))
    cradle = cradle.union(
        box_at(mount_x - 4.0, y0 - 8.0, center_z - 10.0, 8.0, 8.2, 20.0)
    )
    for z in (center_z - 6.0, center_z + 6.0):
        cradle = cradle.cut(cylinder_y(1.4, 12.0, mount_x, outer_y, z))

    return cradle


def make_reference_servo(
    p: Params, pivot: tuple[float, float, float]
) -> tuple[cq.Workplane, cq.Workplane]:
    px, py, pz = pivot
    body_center_x = px + p.servo_body_length / 2 - p.servo_shaft_from_front
    plate_y = py + 5.0
    body = box_at(
        body_center_x - p.servo_body_length / 2,
        plate_y + p.servo_plate_thickness - 0.8,
        pz - p.servo_body_thickness / 2,
        p.servo_body_length,
        p.servo_body_depth,
        p.servo_body_thickness,
    )
    ears = box_at(
        body_center_x - 16.25,
        plate_y - 2.4,
        pz - p.servo_body_thickness / 2,
        32.5,
        2.4,
        p.servo_body_thickness,
    )
    output_boss = cylinder_y(5.0, 5.0, px, py + 1.8, pz)
    spline = cylinder_y(2.45, 2.6, px, py - 2.0, pz)
    servo = body.union(ears).union(output_boss).union(spline)

    horn = cylinder_y(9.0, 1.8, px, py - 2.9, pz)
    horn = horn.union(cylinder_y(3.4, 3.0, px, py - 3.5, pz))
    return servo, horn


def make_reference_screen(p: Params, arm_end: tuple[float, float, float]) -> cq.Workplane:
    _, _, center_z = arm_end
    center_y = p.screen_center_y
    front_x = -p.screen_face_reach
    pcb = box_at(
        front_x - 5.2,
        center_y - p.pcb_width / 2,
        center_z - p.pcb_height / 2,
        1.6,
        p.pcb_width,
        p.pcb_height,
    )
    glass = box_at(
        front_x - 3.6,
        center_y - p.glass_width / 2,
        center_z - p.glass_height / 2,
        2.7,
        p.glass_width,
        p.glass_height,
    )
    connector = box_at(
        front_x - 13.5,
        center_y - 6.0,
        center_z - 5.0,
        8.3,
        12.0,
        10.0,
    )
    return pcb.union(glass).union(connector)


def export_shape(
    shape: cq.Workplane,
    step_path: Path,
    stl_path: Path,
    print_rotation: tuple | None = None,
) -> None:
    cq.exporters.export(shape, str(step_path))
    printable = shape
    if print_rotation is not None:
        axis, angle = print_rotation
        printable = printable.rotate((0, 0, 0), axis, angle)
    cq.exporters.export(
        ground(printable),
        str(stl_path),
        tolerance=0.08,
        angularTolerance=0.18,
    )


def compound(shapes: list[cq.Workplane]) -> cq.Workplane:
    return cq.Workplane(obj=cq.Compound.makeCompound([shape.val() for shape in shapes]))


def validate_shape(name: str, shape: cq.Workplane) -> dict[str, object]:
    bbox = shape.val().BoundingBox()
    return {
        "name": name,
        "valid": shape.val().isValid(),
        "solids": len(shape.solids().vals()),
        "bbox_mm": [round(bbox.xlen, 2), round(bbox.ylen, 2), round(bbox.zlen, 2)],
    }


def intersection_volume(first: cq.Workplane, second: cq.Workplane) -> float:
    common = first.intersect(second)
    return common.val().Volume() if common.val() is not None else 0.0


def build(output: Path, p: Params) -> list[dict[str, object]]:
    output.mkdir(parents=True, exist_ok=True)
    for stale in output.glob("*"):
        if stale.is_file():
            stale.unlink()

    base, pivot = make_base(p)
    wedge = make_wedge(p)
    gauge = make_servo_fit_gauge(p)
    arm, arm_end = make_arm(p, pivot)
    cradle = make_cradle(p, arm_end)
    servo, horn = make_reference_servo(p, pivot)
    reference = make_reference_screen(p, arm_end)

    parts = {
        "brim_servo_clamp": (base, None),
        "clamp_wedge": (wedge, None),
        "mg90s_fit_gauge": (gauge, ((1, 0, 0), 90.0)),
        "servo_display_arm": (arm, ((1, 0, 0), 90.0)),
        "screen_cradle": (cradle, ((0, 1, 0), 90.0)),
    }
    report = []
    for name, (shape, rotation) in parts.items():
        export_shape(
            shape,
            output / f"{name}.step",
            output / f"{name}.stl",
            rotation,
        )
        report.append(validate_shape(name, shape))

    down = compound([base, wedge, servo, horn, arm, cradle, reference])
    cq.exporters.export(down, str(output / "assembly_screen_down.step"))
    cq.exporters.export(down, str(output / "preview_screen_down.stl"), tolerance=0.1)

    axis_start = pivot
    axis_end = (pivot[0], pivot[1] + 1.0, pivot[2])
    moving = [
        shape.rotate(axis_start, axis_end, p.park_angle)
        for shape in (horn, arm, cradle, reference)
    ]
    parked = compound([base, wedge, servo, *moving])
    cq.exporters.export(parked, str(output / "assembly_screen_parked.step"))
    cq.exporters.export(parked, str(output / "preview_screen_parked.stl"), tolerance=0.1)

    parked_arm = moving[1]
    parked_cradle = moving[2]
    clearance_checks = {
        "servo_body_to_clamp": intersection_volume(servo, base),
        "down_arm_to_horn": intersection_volume(arm, horn),
        "down_arm_to_clamp": intersection_volume(arm, base),
        "down_arm_to_servo": intersection_volume(arm, servo),
        "down_cradle_to_clamp": intersection_volume(cradle, base),
        "parked_arm_to_clamp": intersection_volume(parked_arm, base),
        "parked_arm_to_servo": intersection_volume(parked_arm, servo),
        "parked_cradle_to_clamp": intersection_volume(parked_cradle, base),
    }
    collisions = {
        name: volume for name, volume in clearance_checks.items() if volume > 0.01
    }
    if collisions:
        raise RuntimeError(f"Assembly collision check failed: {collisions}")

    (output / "parameters.json").write_text(json.dumps(asdict(p), indent=2) + "\n")
    (output / "validation.json").write_text(json.dumps(report, indent=2) + "\n")
    (output / "clearance_checks.json").write_text(
        json.dumps(clearance_checks, indent=2) + "\n"
    )
    return report


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, default=Path(__file__).parent / "output")
    args = parser.parse_args()
    report = build(args.output, Params())
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
