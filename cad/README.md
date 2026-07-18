# Servo Cap Display Mount

This is a parametric PLA mount for the Axiometa AX22-0034 0.96-inch display and an MG90S micro servo. It clamps to the right-front edge of a baseball-cap brim, raises the display under servo power, and holds the screen facing the right eye approximately 70 mm beyond the brim edge.

## Printed parts

- `brim_servo_clamp.stl`: rigid upper clamp and side-mounted MG90S frame
- `clamp_wedge.stl`: tapered lower jaw for different brim thicknesses
- `mg90s_fit_gauge.stl`: quick aperture and mounting-hole test before the full clamp
- `servo_display_arm.stl`: dropped arm with a universal servo-horn interface
- `screen_cradle.stl`: open-backed snap cradle for the AX22-0034

The STEP files use assembly coordinates. The STL files are rotated and grounded for printing. `assembly_screen_down.step` and `assembly_screen_parked.step` show the two powered positions. The assembly files include reference solids for the servo, horn, and display; those reference items are not separate print files.

## Hardware

- 1x MG90S micro servo with its supplied horn, center screw, and mounting screws
- 2x M3 x 10 mm screws for the cradle-to-arm joint; they self-tap into 2.8 mm PLA pilot holes
- Thin foam tape, electrical tape, or fabric tape for the clamp contact faces
- Two small zip ties for the display cable
- A regulated 5 V supply capable of at least 1 A for the servo, with common ground to the controller

The arm fits the common round, cross, and double MG90S horns. Use the center spline screw and at least two of the horn's small outer screws through the radial slots. MG90S clones vary; if your horn holes do not line up, mark through the horn and drill 1.5 mm pilot holes in the PLA arm root.

## Printing

- Material: PLA
- Nozzle: 0.4 mm
- Layer height: 0.2 mm
- Walls: 4
- Top/bottom layers: 5
- Infill: 30-40% gyroid or cubic
- Supports: build-plate-only under the servo frame; the other parts should not need supports
- Brim: 5 mm on the clamp and servo arm

Print `mg90s_fit_gauge.stl` first. The servo body should pass through without forcing it, and the two mounting pilots should align with the servo ears. Enlarge the body opening in `Params` if your clone is larger.

The default wedge handles a brim approximately 3-5.5 mm thick. Add a layer of foam tape before tightening the wedge; it protects the cap and compensates for the visor curvature visible in the reference photos.

## Assembly

1. Put foam tape inside the upper clamp and on top of the wedge.
2. Slide the clamp over the right-front quadrant of the cap brim.
3. Push the wedge under the brim until the clamp cannot twist. Do not hammer it in.
4. Set the unplugged servo to its mid position by gently turning the fitted horn, then remove the horn.
5. Push the MG90S body through the rectangular frame from the outside. The output shaft faces away from the cap. Use the supplied mounting screws in the two 1.7 mm PLA pilot holes.
6. Fasten the supplied round or cross horn to the printed arm through at least two radial slots.
7. With the arm in the screen-down position, fit the horn to the servo spline and install the center screw.
8. Push the display into the cradle from the open back. The glass faces the bezel and the AX22 connector exits through the open rear corner.
9. Attach the cradle to the arm with two M3 screws.
10. Route the display cable through the arm holes, leaving a loose service loop beside the servo.
11. Power the servo from a regulated 5 V rail, connect controller and servo grounds, and connect the signal wire to a PWM-capable output.
12. Start with a narrow software range around neutral and increase it gradually until down and parked positions are correct.

Do not power the MG90S from a 3.3 V GPIO pin. Its published stall current is approximately 0.75-0.86 A, so use a separate regulated 5 V supply and common ground. Do not command the servo into the printed stops; stalled servos heat quickly and can reset the controller.

The screen is opaque and intentionally occupies the right-eye view only while lowered. Raise it completely before walking, using stairs, cycling, or driving. This is a direct-view display, not an optical see-through HUD.

## Regenerate

```bash
python cad/cap_hud_mount.py
python cad/render_assembly.py cad/previews/screen_down.png
python cad/render_assembly.py cad/previews/screen_parked.png --parked
```

The main tuning values are at the top of `cap_hud_mount.py`. Change `screen_face_reach`, `screen_drop`, `park_angle`, or the wedge gaps after the first physical fit test.

## References

- Axiometa AX22-0034 display: <https://www.axiometa.io/products/ips-lcd-0-96>
- MG90S mechanical and electrical data sheet: <https://www.tinytronics.nl/product_files/000263_Data_Sheet_of_MG90S_Analog_Servo_Motor.pdf>
