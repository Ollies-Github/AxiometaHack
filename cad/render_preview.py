#!/usr/bin/env python3
"""Render generated STL previews using VTK's off-screen renderer."""

from __future__ import annotations

import argparse
from pathlib import Path

import vtk


def render(source: Path, destination: Path, azimuth: float, elevation: float) -> None:
    reader = vtk.vtkSTLReader()
    reader.SetFileName(str(source))
    reader.Update()

    mapper = vtk.vtkPolyDataMapper()
    mapper.SetInputConnection(reader.GetOutputPort())

    actor = vtk.vtkActor()
    actor.SetMapper(mapper)
    actor.GetProperty().SetColor(0.14, 0.62, 0.78)
    actor.GetProperty().SetEdgeColor(0.03, 0.12, 0.16)
    actor.GetProperty().EdgeVisibilityOn()
    actor.GetProperty().SetLineWidth(0.7)

    renderer = vtk.vtkRenderer()
    renderer.SetBackground(0.035, 0.04, 0.045)
    renderer.AddActor(actor)
    renderer.SetUseDepthPeeling(True)
    renderer.ResetCamera()

    camera = renderer.GetActiveCamera()
    camera.Azimuth(azimuth)
    camera.Elevation(elevation)
    camera.Dolly(1.15)
    renderer.ResetCameraClippingRange()

    window = vtk.vtkRenderWindow()
    window.SetOffScreenRendering(1)
    window.SetSize(1400, 1000)
    window.AddRenderer(renderer)
    window.Render()

    capture = vtk.vtkWindowToImageFilter()
    capture.SetInput(window)
    capture.SetScale(1)
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
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    parser.add_argument("--azimuth", type=float, default=-35.0)
    parser.add_argument("--elevation", type=float, default=24.0)
    args = parser.parse_args()
    render(args.source, args.destination, args.azimuth, args.elevation)


if __name__ == "__main__":
    main()
