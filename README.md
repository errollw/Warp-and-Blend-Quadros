# Warping and Blending using NVAPI

This project hosts the code for *warping and blending* our tiled projector display wall in the [Rainbow Laboratory](http://www.cl.cam.ac.uk/research/rainbow/) at the [Cambridge Computer Lab](http://www.cl.cam.ac.uk/). We use Nvidia’s GPU utilities and programming interface ([NVAPI](https://developer.nvidia.com/nvapi)) to avoid struggling with many issues faced by past multi-projector deployments.

![Image of our display wall](http://i.imgur.com/5S2TnZ3l.jpg)

### Projector hardware

Our design uses six [BenQ W1400](http://benq.co.uk/product/projector/w1400) commodity-level projectors (1920×1080px each) providing a maximum resolution of **12.4 megapixels**. Our screen is sized at 4×1.5m, so users can get up to 2.5m close to the screen without being able to discern individual pixels. The projectors are arranged in a 3×2 configuration, and mounted under horizontal scaffolding beams attached to the ceiling. Our projector models are *short-throw*, the closest ones are 1.45m away from the screen so users can approach the display without creating unwanted shadows. They also feature *off-axis projection* and *lens-shift*, allowing them to be placed 2m high without experiencing serious perspective distortion.

They are driven by a Windows 8.1 PC with two quad-core 1.80GHz Intel
Xeon E5-2603 processors, two [NVIDIA Quadro K5000](http://www.nvidia.co.uk/object/quadro-k5000-uk.html) GPUs, and 16GB RAM. Our display uses this specialist hardware to appear as a single screen to the operating system, avoiding multiple-display issues.

### Projector calibration

In multi-projector setups, there are two common calibration steps required to make a seamless displays:

1. **Warping** - The displays must be aligned so projected regions cover the desired area without distortion .
2. **Blending** - Intensity correction must be applied to overlapping regions to avoid illumination irregularities.

We chose manual calibration to avoid additional hardware and system complexity. Using the calibration utility `util\calibrate_screens.html`, users manually transform and align six on-screen quadrilaterals using keyboard and mouse. This then outputs transform coordinates for each projector that can be used for warping and blending.

The blending textures are generated with `warp_blending_mask.py` and [OpenCV](http://opencv.org/). We first calculate the homography between an unwarped rectangle and the warped projected quadrilateral, and then perform a perspective warp on an unwarped blending mask.

### NVIDIA API usage

*Nvidia Mosaic* is a utility that allows the operating system to view multiple displays as a single unified desktop environment without degrading performance or needing to modify third-party software. It also allows users to specify projector overlap regions so the desktop display transitions seamlessly over the entire display without discontinuities.

*NVAPI Quadro Warp/Blend* lets us warp displays with a warping mesh to remove keystone distortion. In our case, each projected display has a quadrilateral warping mesh. These transformations are done in the display pipeline before pixels are scanned out, and leverage GPU hardware to perform fast high-quality image filtering operations. It also allows a blending texture to be loaded and applied to each display which modifies the output intensity of each pixel.


### Using the calibration utility

Prerequisites:

* You'll need a copy of Visual Studio to compile the code
* You need to download a copy of [the NVidia NVAPI library](https://developer.nvidia.com/nvapi).
* Using these to compile the two utilities, *WarpBlend-Quadros* and *UnwarpAll-Quadros*, is left as an exercise for the reader!

Once you've done this...

* The code currently assumes 6 projectors: 3 columns across and 2 rows
* Your displays need to be connected in the correct order so that the Nvidia utilities number them 1-6, reading from top left to bottom right.
* Set up Mosaic using the Nvidia Control Panel to put the displays in the right basic configuration, and enable it.  Windows should now consider it has one big display attached.
* If you have any existing warping applied, make sure you run *UnwarpAll-Quadros* to clear it before calibrating.
* Open the `calibrate_screens.html` page in Chrome, or in any other browser that will let you display it full-screen and give you access to the console log.
* Six coloured quadrilaterals will appear, each should be positions basically on one projector, and your aim is to move their corner points so that the corners on adjacent quadrilaterals are aligned.  The controls for doing this are described below.
* When you have aligned everything, the necessary coordinates will be output to the browser console log.  Copy these into a text file named `coords_for_warp.txt` and run the *WarpBlend-Quadros* utility, and it will use these to set up the warping and blending.

Controls for the calibration screen:

*  Click in one of the rectangles to select the nearest corner of that rectangle.  The corner will turn white.
*  Ctrl-click to move it to approximately the right location.  You want to position each corner roughly in the middle of the overlapping zone of the projectors.
*  Use cursor keys to make fine adjustments to the selected corner, and then move on to the next corner.
*  You can use the Q, W, E, A, S, & D keys to toggle on or off the rectangle displayed at each corresponding location.
*  You can use the shift key with the cursor keys to change the size of the overlap area.
*  Finally, press 'P', and the coordinates will be printed to the browser's console log; you can copy them into a text file.


