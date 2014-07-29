import cv2, numpy as np

# Constants
H_BLEND_PX = 30;
V_BLEND_PX = 90;
GAMMA = 2.2;

# The original scanout coordinates
orig_coords = [(0,0),    (1920,0),
               (0,1080), (1920,1080)]

# The warped corrected coordinates
warp_coords = {
	(0,0) : [(   36.00,     0.00),	( 1862.00,     1.01), 
             (    0.51,  1068.98),	( 1913.17,  1062.90)],
    (0,1) : [(   52.00,     0.00),	( 1860.00,     0.00), 
             (    0.86,  1076.08),	( 1919.52,  1058.79)],
    (0,2) : [(   62.00,     0.00),	( 1855.00,     0.00), 
             (   18.16,  1071.17),	( 1919.78,  1044.91)],
    (1,0) : [(    8.75,    31.12),	( 1911.48,    15.95), 
             (   79.00,  1002.00),	( 1859.00,   967.71)],
    (1,1) : [(    7.04,    26.14),	( 1914.98,    21.06), 
             (   70.00,   981.10),	( 1852.00,   968.90)],
    (1,2) : [(   21.90,    16.16),	( 1921.81,     9.09), 
             (   88.00,   974.08),	( 1862.00,   965.00)]       
}


def make_blend_img(screen_idx):

	pre_warp_blend_img = np.ones(shape=(1080,1920), dtype=np.float32)

	if screen_idx[1] in [1,2]:
		for col in range(H_BLEND_PX):
			pre_warp_blend_img[:,col] *= (col / float(H_BLEND_PX)) ** (1/GAMMA)

	if screen_idx[1] in [0,1]:
		for col in range(H_BLEND_PX):
			pre_warp_blend_img[:,-col-1] *= (col / float(H_BLEND_PX)) ** (1/GAMMA)

	if screen_idx[0] == 0:
		for row in range(V_BLEND_PX):
			pre_warp_blend_img[-row,:] *= (row / float(V_BLEND_PX)) ** (1/GAMMA)

	if screen_idx[0] == 1:
		for row in range(V_BLEND_PX):
			pre_warp_blend_img[row,:] *= (row / float(V_BLEND_PX)) ** (1/GAMMA)

	return pre_warp_blend_img


def warp_blend_img(pre_blend_img, screen_idx):

	points1 = np.array(orig_coords, dtype=float)
	points2 = np.array(warp_coords[screen_idx], dtype=float)

	(homography, _) = cv2.findHomography(points1, points2, method=cv2.RANSAC)
	homography = np.matrix(homography)

	warped_blend_img = cv2.warpPerspective(pre_blend_img, homography, (1920,1080))

	return warped_blend_img


##########################
### SCRIPT STARTS HERE ###
##########################

for screen_idx in warp_coords.keys():

	pre_blend_img = make_blend_img(screen_idx)
	warped_blend_img = warp_blend_img(pre_blend_img, screen_idx)
	cv2.imshow("warped_blend_img",warped_blend_img)
	cv2.imwrite("blend_"+str(screen_idx)+".png", warped_blend_img*255)

print "FINISHED"
