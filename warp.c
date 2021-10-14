//
// -*- coding: utf-8 -*-
// $Date: 2021/10/14 21:30:52 $
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

// 単位は m と rad とする。

int main(int argc, char *argv[])
{
	// 引数
	unsigned int idebug = 0;
	double h_mm = 0;

	for(int i = 0; i < argc; i++){
		printf("argv[%d]\t: %s\n", i, argv[i]);

		if(strncmp(argv[i], "0x", 2) == 0){
			idebug = strtol(argv[i], NULL, 16);
		}else if(strcspn(argv[i], "0123456789.+-eE") != strlen(argv[i])){
			h_mm = atof(argv[i]);
		}
	}

	printf("idebug\t: 0x%02X\n", idebug);
	printf("h_mm\t: %+.3lf\n", h_mm);

	printf("--------------------------------\n");

	// 魚眼の焦点距離
	const double f = 16e-3;
	printf("f\t: %+.3lf mm\n", f * 1000);

	// 衛星の高度
	const double A = 400e3;
	printf("A\t: %+.3lf km\n", A / 1000);

	// 地球の半径
	const double R = 6371e3;
	printf("R\t: %+.3lf km\n", R / 1000);

	printf("--------------------------------\n");

	// 地平線
	const double theta_max = asin(R / (A+R));
	const double D_max = R * (M_PI / 2 - theta_max);
	const double h_max = 2 * f * sin(theta_max / 2);
	const double phi_max = M_PI / 2  - theta_max;
	printf("theta_max\t: %+.3lf deg\n", theta_max / M_PI * 180);
	printf("D_max\t: %+.3lf km\n", D_max / 1000);
	printf("h_max\t: %+.3lf mm\n", h_max * 1000);
	printf("phi_max\t: %+.3lf deg\n", phi_max / M_PI * 180);

	printf("--------------------------------\n");

	// 像高
	const double h = h_mm ? h_mm/1000 : h_max;//18.4e-3;
	printf("h\t: %+.3lf mm\n", h * 1000);

	// 入射角
	const double theta = 2 * asin(h / 2 / f);
	printf("theta\t: %+.3lf deg\n", theta / M_PI * 180);

	printf("--------------------------------\n");

	// 中間変数
	const double t = 1 / tan(theta);
	const double s = A/R + 1;
	printf("t\t: %+.3lf\n", t);
	printf("s\t: %+.3lf\n", s);

	printf("--------------------------------\n");

	// 二次方程式
	const double a = t*t + 1;
	const double bd = -s;
	const double c = s*s - t*t;
	const double dd = bd*bd - a * c;
	printf("a\t: %+.3lf\n", a);
	printf("b'\t: %+.3lf\n", bd);
	printf("c\t: %+.3lf\n", c);
	printf("d'\t: %+.3lf\n", dd);

	const double cos_phi = (-bd + sqrt(dd)) / a;
	printf("cos_phi\t: %+.3lf\n", cos_phi);

	printf("--------------------------------\n");

	// 地球の角度
	const double phi = acos(cos_phi);
	printf("phi\t: %+.3lf deg\n", phi / M_PI * 180);

	// 地表の距離
	const double D = R * phi;
	printf("D\t: %+.3lf km\n", D / 1000);

	exit(EXIT_SUCCESS);
}



/*



tyasu@tyasuda-dell MINGW64 /d/gyogan
$ gcc warp.c

tyasu@tyasuda-dell MINGW64 /d/gyogan
$ ./a 0x11 18
argv[0] : C:\Users\tyasu\OneDrive\▒h▒L▒▒▒▒▒▒▒g\gyogan\a.exe
argv[1] : 0x11
argv[2] : 18
idebug  : 0x11
h_mm    : +18.000
--------------------------------
f       : +16.000 mm
A       : +400.000 km
R       : +6371.000 km
--------------------------------
theta_max       : +70.207 deg
D_max   : +2200.836 km
h_max   : +18.402 mm
phi_max : +19.793 deg
--------------------------------
h       : +18.000 mm
theta   : +68.458 deg
--------------------------------
t       : +0.395
s       : +1.063
--------------------------------
a       : +1.156
b'      : -1.063
c       : +0.974
d'      : +0.004
cos_phi : +0.975
--------------------------------
phi     : +12.862 deg
D       : +1430.182 km

tyasu@tyasuda-dell MINGW64 /d/gyogan
$



*/
