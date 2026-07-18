#!/usr/bin/env python3
"""Render color-coded down and parked assembly previews."""

from __future__ import annotations

import argparse
from pathlib import Path

import cadquery as cq
import vtk

from cap_hud_mount import (
    Params,
    make_arm,
    make_base,
    make_cradle,
    make_reference_screen,
    make_reference_servo,
    make_wedge,
)


def actor_for(shape: cq.Workplane, color: tuple[float, float, float]) -> vtk.vtkActor:
    vertices, triangles = shape.val().tessellate(0.12, 0.2)
    points = vtk.vtkPoints()
    for vertex in vertices:
        points.InsertNextPoint(vertex.x, vertex.y, vertex.z)

    cells = vtk.vtkCellArray()
    for triangle in triangles:
        cell = vtk.vtkTriangle()
        for index, vertex_index in enumerate(triangle):
            cell.GetPointIds().SetId(index, vertex_index)
        cells.InsertNextCell(cell)

    mesh = vtk.vtkPolyData()
    mesh.SetPoints(points)
    mesh.SetPolys(cells)
    normals = vtk.vtkPolyDataNormals()
    normals.SetInputData(mesh)
    normals.SetFeatureAngle(50)
    normals.SplittingOn()

    mapper = vtk.vtkPolyDataMapper()
    mapper.SetInputConnection(normals.GetOutputPort())
    actor = vtk.vtkActor()
    actor.SetMapper(mapper)
    actor.GetProperty().SetColor(*color)
    actor.GetProperty().SetEdgeColor(0.025, 0.04, 0.05)
    actor.GetProperty().EdgeVisibilityOn()
    actor.GetProperty().SetLineWidth(0.55)
    return actor


def render(destination: Path, parked: bool) -> None:
    p = Params()
    base, pivot = make_base(p)
    wedge = make_wedge(p)
    arm, arm_end = make_arm(p, pivot)
    cradle = make_cradle(p, arm_end)
    servo, horn = make_reference_servo(p, pivot)
    screen = make_reference_screen(p, arm_end)

    if parked:
        axis_end = (pivot[0], pivot[1] + 1.0, pivot[2])
        horn, arm, cradle, screen = [
            shape.rotate(pivot, axis_end, p.park_angle)
            for shape in (horn, arm, cradle, screen)
        ]

    shapes = [base, wedge, servo, horn, arm, cradle, screen]
    colors = [
        (0.12, 0.55, 0.72),
        (0.72, 0.76, 0.78),
        (0.25, 0.27, 0.29),
        (0.96, 0.48, 0.12),
        (0.10, 0.66, 0.76),
        (0.13, 0.61, 0.70),
        (0.35, 0.86, 0.43),
    ]

    renderer = vtk.vtkRenderer()
    renderer.SetBackground(0.035, 0.04, 0.045)
    for shape, color in zip(shapes, colors):
        renderer.AddActor(actor_for(shape, color))

    bounds = [shape.val().BoundingBox() for shape in shapes]
    xmin = min(bound.xmin for bound in bounds)
    xmax = max(bound.xmax for bound in bounds)
    ymin = min(bound.ymin for bound in bounds)
    ymax = max(bound.ymax for bound in bounds)
    zmin = min(bound.zmin for bound in bounds)
    zmax = max(bound.zmax for bound in bounds)
    center = ((xmin + xmax) / 2, (ymin + ymax) / 2, (zmin + zmax) / 2)

    camera = renderer.GetActiveCamera()
    camera.SetFocalPoint(*center)
    camera.SetPosition(center[0] + 160, center[1] - 260, center[2] + 120)
    camera.SetViewUp(0, 0, 1)
    camera.ParallelProjectionOn()
    width_for_aspect = (xmax - xmin) + 0.6 * (ymax - ymin)
    camera.SetParallelScale(max((zmax - zmin) * 0.62, width_for_aspect * 0.36))
    renderer.ResetCameraClippingRange()

    window = vtk.vtkRenderWindow()
    window.SetOffScreenRendering(1)
    window.SetSize(1400, 950)
    window.AddRenderer(renderer)
    window.Render()

    capture = vtk.vtkWindowToImageFilter()
    capture.SetInput(window)
    capture.SetInputBufferTypeToRGBA()
    capture.ReadFrontBufferOff()
    capture.Update()

    destination.parent.mkdir(parents=True, exist_ok=True)
    writer = vtk.vtkPNGWriter()
    writer.SetFileName(str(destination))
    writer.SetInputConnection(capture.GetOutputPort())
    writer.Write()
    window.Finalize()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("destination", type=Path)
    parser.add_argument("--parked", action="store_true")
    args = parser.parse_args()
    render(args.destination, args.parked)


if __name__ == "__main__":
    main()
