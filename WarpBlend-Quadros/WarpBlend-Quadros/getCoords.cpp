#include "stdafx.h"

// for reading a file into a string
#include <string>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <windows.h>

#include <regex>

#include <Eigen\Geometry>

using namespace std;
using namespace Eigen;

vector<float> get_warping_vertices(int row, int col, float srcLeft, float srcTop, float srcWidth, float srcHeight) {

	// read coordinates file into str
	std::ifstream f("coords_for_warp.txt");
	if (!f.is_open()) {
		std::cout << "Failed to open coords file\n";
		throw "Failed to open coords file";
	}
	std::string str((std::istreambuf_iterator<char>(f)),
					 std::istreambuf_iterator<char>());

	printf("Using warping:\n----------------\n%s\n----------------\n", str.c_str());
	// extract float coordinates from text using regex
	std::regex regex(
		"ROW: " + to_string(row) + " COL: " + to_string(col) + "\\n"
		"\\(\\s*(\\-?[0-9]+.[0-9]+),\\s*(\\-?[0-9]+.[0-9]+)\\)\\s+"		// top left corner (x,y)
		"\\(\\s*(\\-?[0-9]+.[0-9]+),\\s*(\\-?[0-9]+.[0-9]+)\\)\\s*\\n"	// top right corner (x,y)
		"\\(\\s*(\\-?[0-9]+.[0-9]+),\\s*(\\-?[0-9]+.[0-9]+)\\)\\s+"		// bottom left corner (x,y)
		"\\(\\s*(\\-?[0-9]+.[0-9]+),\\s*(\\-?[0-9]+.[0-9]+)\\)"			// bottom right corner (x,y)
		);
	std::smatch match;
	std::regex_search(str, match, regex);

	std::cout << "coords found:" << match.size() << std::endl;

	// extract coordinates of quadrilateral corners
	Vector2f tl = Vector2f(stof(match[1].str()), stof(match[2].str()));
	Vector2f tr = Vector2f(stof(match[3].str()), stof(match[4].str()));
	Vector2f bl = Vector2f(stof(match[5].str()), stof(match[6].str()));
	Vector2f br = Vector2f(stof(match[7].str()), stof(match[8].str()));

	// find intersection between diagonals
	Hyperplane<float,2> tl_br = Hyperplane<float,2>::Through(tl,br);
	Hyperplane<float,2> tr_bl = Hyperplane<float,2>::Through(tr,bl);
	Vector2f intersection = tl_br.intersection(tr_bl);

	// quadrilateral projective interpolation
	// FIXME: this isn't working, non-continuous seams
	// see: http://www.reedbeta.com/blog/2012/05/26/quadrilateral-interpolation-part-1/

	// calculate distances from vertices to intersection
	float d_tl = (tl-intersection).norm();
	float d_tr = (tr-intersection).norm();
	float d_bl = (bl-intersection).norm();
	float d_br = (br-intersection).norm();

	// fourth texture coordinate 'q' plays the role of '1/z' in texturing
	// FIXME - this projective interpolation does not work across seams
	// FIXME - we need some sort of bilateral interpolation
	// float q_tl = (d_tl + d_br) / (d_br);
	// float q_tr = (d_tr + d_bl) / (d_bl);
	// float q_bl = (d_bl + d_tr) / (d_tr);
	// float q_br = (d_br + d_tl) / (d_tl);

	// instead, treat quad as just a trapesium, calculate q for middle screens only
	float q = (row == 0) ? (1860-52)/(1919.52-0.86) : (1914.98-7.04)/(1852-70);
	float q_tl = q;
	float q_tr = q;
	float q_bl = 1;
	float q_br = 1;

	// XYUVRW coordinates to return
	//  (0)  ----------------- (2)
    //       |             / |
    //       |           /   |
    //       |         /     |
    //       |       /       |
    //       |     /         |
    //       |   /           |
    //       | /             |  
    //   (1) ----------------- (3)

	float coords[] = {
		tl.x(),     tl.y(),     srcLeft * q_tl,				srcTop * q_tl,				0.0f,    q_tl,	// 0
        bl.x(),		bl.y(),     srcLeft * q_bl,				(srcTop+srcHeight) * q_bl,  0.0f,    q_bl,	// 1
        tr.x(),		tr.y(),     (srcWidth+srcLeft) * q_tr,	srcTop * q_tr,				0.0f,    q_tr,	// 2
        br.x(),     br.y(),		(srcWidth+srcLeft) * q_br,  (srcTop+srcHeight) * q_br,  0.0f,    q_br	// 3
	};

	vector<float> coords_vector(coords,coords+sizeof(coords)/sizeof(coords[0]));

	return coords_vector;
}