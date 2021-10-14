//
// -*- coding: utf-8 -*-
// $Date: 2021/10/15 00:44:48 $
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>



// 単位は m と rad とする。



// センサーサイズ
const double W = 36e-3;
const double H = 24e-3;

// 画素サイズ的なもの
const double h_eps = 10e-6;

// 魚眼の焦点距離
const double f = 16e-3;

// 衛星の高度
const double A = 400e3;

// 地球の半径
const double R = 6371e3;



double warp(double h)
{
	// 入射角
	const double theta = 2 * asin(h / 2 / f);

	// 中間変数
	const double t = 1 / tan(theta);
	const double s = A/R + 1;

	// 二次方程式
	const double a = t*t + 1;
	const double bd = -s;
	const double c = s*s - t*t;
	const double dd = bd*bd - a * c;

	const double cos_phi = (-bd + sqrt(dd)) / a;

	// 地球の角度
	const double phi = acos(cos_phi);

	// 地表の距離
	const double D = R * phi;

	return D;
}

int main(int argc, char *argv[])
{
	// 引数
	unsigned int idebug = 0;

	for(int i = 0; i < argc; i++){
		printf("argv[%d]\t: %s\n", i, argv[i]);

		if(strncmp(argv[i], "0x", 2) == 0){
			idebug = strtol(argv[i], NULL, 16);
		}else if(strcspn(argv[i], "0123456789.+-eE") != strlen(argv[i])){
			;;;
		}
	}

	// 地平線
	const double theta_max = asin(R / (A+R));
	const double D_max = R * (M_PI / 2 - theta_max);
	const double h_max = 2 * f * sin(theta_max / 2);
	const double phi_max = M_PI / 2  - theta_max;

	// センサーの第一象限サイズ
	const uint16_t ix_max_um = (W/2/h_eps) + 0.5;
	const uint16_t iy_max_um = (H/2/h_eps) + 0.5;

	// ワープマップ
	const size_t iwarp_num = ix_max_um * iy_max_um;
	double *warp_u = (double *)calloc(sizeof(double), iwarp_num);
	double *warp_v = (double *)calloc(sizeof(double), iwarp_num);

	if(warp_u == NULL || warp_v == NULL){
		printf("oops! warp_u/v short of memory\n");
		exit(EXIT_FAILURE);
	}

	if(idebug & 0x01){
		printf("iwarp_num\t: %u\n", iwarp_num);
		fflush(stdout);
	}

	// ワープマップ作成
	for(uint16_t iy_um = 0; iy_um < iy_max_um; iy_um++){
		for(uint16_t ix_um = iy_um; ix_um < ix_max_um; ix_um++){
			// 像高
			double h = sqrt(ix_um * ix_um + iy_um * iy_um) * h_eps;

			if(h_max < h){
				break;
			}

			double d = warp(h);

			double u_km = ix_um / h * d * h_eps / 1000;
			double v_km = iy_um / h * d * h_eps / 1000;

			size_t i = iy_um * ix_max_um + ix_um;
			warp_u[i] = u_km;
			warp_v[i] = v_km;

			if(ix_um < iy_max_um){
				size_t i = ix_um * ix_max_um + iy_um;
				warp_u[i] = v_km;
				warp_v[i] = u_km;
			}
		}
	}

	// ワープマップ保存
	if(0){
		const char *file_warp= "warp_map_verify.ppm";
		FILE *fp_warp = fopen(file_warp, "w");

		if(fp_warp == NULL){
			printf("oops! %s\n", file_warp);
			exit(EXIT_FAILURE);
		}

		const uint16_t iD_max_km = D_max / 1000;
		fprintf(fp_warp, "P3\n");
		fprintf(fp_warp, "%u %u\n", ix_max_um, iy_max_um);
		fprintf(fp_warp, "%u\n", iD_max_km);	// 2^16 = 65536

		for(uint16_t iy_um = 0; iy_um < iy_max_um; iy_um++){
			for(uint16_t ix_um = 0; ix_um < ix_max_um; ix_um++){
				size_t i = iy_um * ix_max_um + ix_um;
				double u_km = warp_u[i];
				double v_km = warp_v[i];
				uint16_t inodata = 0;

				if(u_km == 0 && v_km == 0){
					inodata = iD_max_km;
				}

				fprintf(fp_warp, "%u %u %u ", u_km, v_km, inodata);
			}

			fprintf(fp_warp, "\n");
		}

		fclose(fp_warp);
		exit(EXIT_SUCCESS);
	}



	// 地図画像読込
	char *file_r = "map.ppm";
	FILE *fp_r = fopen(file_r, "r");
	if(fp_r == NULL){ printf("oops! R %s\n", file_r); exit(EXIT_FAILURE); }
	uint32_t imap_w = 6944;
	uint32_t imap_h = 4432;

	char buff[256];
	fgets(buff, 256, fp_r);	// P3
	fscanf(fp_r, "%lu %lu", &imap_w, &imap_h);	// 6944 4432
	fscanf(fp_r, "%s", buff);					// 255

	size_t imap_num = (imap_w * imap_h) + (32-(imap_w * imap_h) % 32);
	uint8_t *imap_cr = (uint8_t *)calloc(sizeof(uint8_t), imap_num);
	uint8_t *imap_cg = (uint8_t *)calloc(sizeof(uint8_t), imap_num);
	uint8_t *imap_cb = (uint8_t *)calloc(sizeof(uint8_t), imap_num);

	if(imap_cr == NULL || imap_cg == NULL || imap_cb == NULL){
		printf("oops! short of memory\n");
		exit(EXIT_FAILURE);
	}

//	const uint16_t map_scale = 16;
//	const uint16_t map_scale = 8;
//	const uint16_t map_scale = 5;
//	const uint16_t map_scale = 4;
//	const uint16_t map_scale = 3;
//	const uint16_t map_scale = 2;
//	const double   map_scale = 1.5;
//	const uint16_t map_scale = 1;
	const double   map_scale = 0.8;
//	const double   map_scale = 0.5;

	if(idebug & 0x01){
		printf("imap_w(%u)\n", imap_w);
		printf("imap_h(%u)\n", imap_h);
		printf("imap_level_max(%s)\n",buff);
		printf("imap_num(%u)\n",imap_num);
		fflush(stdout);
	}

	size_t i = 0, ix = 0, iy = 0;

	while(feof(fp_r) == 0){
		int icr, icg, icb;

		if(fscanf(fp_r, "%d %d %d", &icr, &icg, &icb) != 3){
			break;
		}

		if(idebug & 0x10){
			printf("(%8u %4u %4u) (%4d %4d %4d)\n", i, ix, iy, icr, icg, icb);
			fflush(stdout);
		}

		imap_cr[i] = (uint8_t)icr;
		imap_cg[i] = (uint8_t)icg;
		imap_cb[i] = (uint8_t)icb;

		if(ix == imap_w-1){
			ix = 0;
			iy++;
		}else{
			ix++;
		}

		i++;
	}

	if(idebug & 0x01){
		size_t imap_used = i;
		double percent = 100.0*imap_used/imap_num;
		printf("imap: %u / %u = %.3lf%%\n", imap_used, imap_num, percent);
	}

	fclose(fp_r);

	//-------------------------------------------------------------------------

//	int16_t imap_u_offset_km = +9360 + 100;
//	int16_t imap_v_offset_km = -360;
//	int16_t imap_u_offset_km = 0;
//	int16_t imap_v_offset_km = -300;

	int16_t imap_u_offset_km = -800;
	int16_t imap_v_offset_km = +800;
	int16_t ifile_cnt = 0;

	while(1){
		char file_w[256] = "fish_0000.ppm";
		sprintf(file_w, "fish_%04d.ppm", ifile_cnt);

		if(idebug & 0x20){
			printf("file\t: %04d (%+04d %+04d)\n", ifile_cnt, imap_u_offset_km, imap_v_offset_km);
			fflush(stdout);
		}

		// 出力画像
		FILE *fp_w = fopen(file_w, "w");

		if(fp_w == NULL){
			printf("oops! W %s\n", file_w);
			exit(EXIT_FAILURE);
		}

		fputs("P3\n", fp_w);
		fputs("3600 2400\n", fp_w);	// W H
		fputs("255\n", fp_w);

		uint8_t icr;
		uint8_t icg;
		uint8_t icb;

		// 上半分
		for(uint16_t iy_um = 1; iy_um <= iy_max_um; iy_um++){

			// 左上
			for(uint16_t ix_um = 1; ix_um <= ix_max_um; ix_um++){
				unsigned ix = ix_max_um - ix_um;	// 左
				unsigned iy = iy_max_um - iy_um;	// 上

				size_t i = iy * ix_max_um + ix;

				if(iwarp_num < i){
					printf("%d\n", i);
					exit(EXIT_FAILURE);
				}

				double u_km = warp_u[i];
				double v_km = warp_v[i];

				if(u_km == 0 && v_km == 0){
					fprintf(fp_w, "%u %u %u ", 0, 0, 0);
				}else{
					size_t imap_u = imap_u_offset_km + imap_w / 2 - u_km / map_scale;	// 左
					size_t imap_v = imap_v_offset_km + imap_h / 2 - v_km / map_scale;	// 上
					size_t i = imap_v * imap_w + imap_u;

					if(imap_num <= i){
						icr = 255;
						icg = 0;
						icb = 0;
					}else{
						icr = imap_cr[i];
						icg = imap_cg[i];
						icb = imap_cb[i];
					}

					fprintf(fp_w, "%u %u %u ", icr, icg, icb);	// R G B
				}
			}

			// 右上
			for(uint16_t ix_um = ix_max_um; ix_um < 2 * ix_max_um; ix_um++){
				unsigned ix = ix_um - ix_max_um;	// 右
				unsigned iy = iy_max_um - iy_um;	// 上

				size_t i = iy * ix_max_um + ix;

				if(iwarp_num < i){
					printf("%d\n", i);
					exit(EXIT_FAILURE);
				}

				double u_km = warp_u[i];
				double v_km = warp_v[i];

				if(u_km == 0 && v_km == 0){
					fprintf(fp_w, "%u %u %u ", 0, 0, 0);
				}else{
					size_t imap_u = imap_u_offset_km + imap_w / 2 + u_km / map_scale;	// 右
					size_t imap_v = imap_v_offset_km + imap_h / 2 - v_km / map_scale;	// 上
					size_t i = imap_v * imap_w + imap_u;

					if(imap_num <= i){
						icr = 255;
						icg = 0;
						icb = 0;
					}else{
						icr = imap_cr[i];
						icg = imap_cg[i];
						icb = imap_cb[i];
					}

					fprintf(fp_w, "%u %u %u ", icr, icg, icb);	// R G B
				}
			}

			fprintf(fp_w, "\n");
		}

		// 下半分
		for(uint16_t iy_um = iy_max_um; iy_um < 2 * iy_max_um; iy_um++){

			// 左下
			for(uint16_t ix_um = 1; ix_um <= ix_max_um; ix_um++){
				unsigned ix = ix_max_um - ix_um;	// 左
				unsigned iy = iy_um - iy_max_um;	// 下

				size_t i = iy * ix_max_um + ix;

				if(iwarp_num < i){
					printf("%d\n", i);
					exit(EXIT_FAILURE);
				}

				double u_km = warp_u[i];
				double v_km = warp_v[i];

				if(u_km == 0 && v_km == 0){
					fprintf(fp_w, "%u %u %u ", 0, 0, 0);
				}else{
					size_t imap_u = imap_u_offset_km + imap_w / 2 - u_km / map_scale;	// 左
					size_t imap_v = imap_v_offset_km + imap_h / 2 + v_km / map_scale;	// 下
					size_t i = imap_v * imap_w + imap_u;

					if(imap_num <= i){
						icr = 255;
						icg = 0;
						icb = 0;
					}else{
						icr = imap_cr[i];
						icg = imap_cg[i];
						icb = imap_cb[i];
					}

					fprintf(fp_w, "%u %u %u ", icr, icg, icb);	// R G B
				}
			}

			// 右下
			for(uint16_t ix_um = ix_max_um; ix_um < 2 * ix_max_um; ix_um++){
				unsigned ix = ix_um - ix_max_um;	// 右
				unsigned iy = iy_um - iy_max_um;	// 下

				size_t i = iy * ix_max_um + ix;

				if(iwarp_num < i){
					printf("%d\n", i);
					exit(EXIT_FAILURE);
				}

				double u_km = warp_u[i];
				double v_km = warp_v[i];

				if(u_km == 0 && v_km == 0){
					fprintf(fp_w, "%u %u %u ", 0, 0, 0);
				}else{
					size_t imap_u = imap_u_offset_km + imap_w / 2 + u_km / map_scale;	// 右
					size_t imap_v = imap_v_offset_km + imap_h / 2 + v_km / map_scale;	// 下
					size_t i = imap_v * imap_w + imap_u;

					if(imap_num <= i){
						icr = 255;
						icg = 0;
						icb = 0;
					}else{
						icr = imap_cr[i];
						icg = imap_cg[i];
						icb = imap_cb[i];
					}

					fprintf(fp_w, "%u %u %u ", icr, icg, icb);	// R G B
				}
			}

			fprintf(fp_w, "\n");
		}

		fclose(fp_w);

//		char cmd[256];
//		sprintf(cmd, "magick.exe convert -resize 50%% %s %s", file_w, file_w);
//		strcat(cmd-4, ".gif");
//		system(cmd);

		if(800 <= imap_u_offset_km){
			break;
		}

		imap_u_offset_km += 10;
		imap_v_offset_km -= 10;
		ifile_cnt++;
	}while(0);
}
