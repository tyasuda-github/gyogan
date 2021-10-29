//
// -*- coding: utf-8 -*-
// $Date: 2021/10/26 02:12:47 $
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>



// 単位は m と rad とする。



// センサーサイズ
// 8192pixel x 2.5um = 20.48mm
const double W = 36e-3;	// 20.48e-3;
const double H = 36e-3;	// 20.48e-3;

// 画素サイズ的なもの
const double px_size = 10e-6;

// 魚眼の焦点距離
const double f = 16e-3;

// 衛星の高度
const double A = 400e3;

// 地球の半径
const double R = 6371e3;



double deg(double rad)
{
	return rad / M_PI * 180;
}



double warp(double h)
{
	// 入射角（半画角）
	const double theta = 2 * asin(h / 2 / f);

	// 中間変数
	const double t = 1 / tan(theta);
	const double s = A/R + 1;

	// 二次方程式の係数と判別式
	const double a = t*t + 1;
	const double bd = -s;
	const double c = s*s - t*t;
	const double dd = bd*bd - a * c;

	const double cos_phi = (-bd + sqrt(dd)) / a;

	// 地球の中心の角度
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
	const uint16_t ix_max_px = (W/2/px_size) + 0.5;
	const uint16_t iy_max_px = (H/2/px_size) + 0.5;

	if(idebug & 0x01){
		printf("theta_max [deg]\t: %+8.3lf\n", deg(theta_max));
		printf("D_max [km]\t: %+8.3lf\n", D_max / 1000);
		printf("h_max [mm]\t: %+8.3lf\n", h_max * 1000);
		printf("phi_max [deg]\t: %+8.3lf\n", deg(phi_max));
		printf("ix_max_px [px]\t: %u\n", ix_max_px);
		printf("iy_max_px [px]\t: %u\n", iy_max_px);
		fflush(stdout);
	}

	// ワープマップ
	const size_t iwarp_num = ix_max_px * iy_max_px;
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
	for(uint16_t iy_px = 0; iy_px < iy_max_px; iy_px++){
		for(uint16_t ix_px = iy_px; ix_px < ix_max_px; ix_px++){
			// 像高
			double h = sqrt(ix_px * ix_px + iy_px * iy_px) * px_size;

			if(h_max < h){
				break;
			}

			double d = warp(h);

			double u_km = ix_px / h * d * px_size / 1000;
			double v_km = iy_px / h * d * px_size / 1000;
//			printf("%u %u %+8.3lf %+8.3lf\n", ix_px, iy_px, u_km, v_km);

			size_t i = iy_px * ix_max_px + ix_px;
			warp_u[i] = u_km;
			warp_v[i] = v_km;

			if(ix_px < iy_max_px){
				size_t i = ix_px * ix_max_px + iy_px;
				warp_u[i] = v_km;
				warp_v[i] = u_km;
			}
		}
	}

	// ワープマップ保存
	if(idebug & 0x10){
		const char *file_warp= "warp_map_verify.ppm";
		FILE *fp_warp = fopen(file_warp, "w");

		if(fp_warp == NULL){
			printf("oops! %s\n", file_warp);
			exit(EXIT_FAILURE);
		}

		const uint16_t iD_max_km = D_max / 1000;
		fprintf(fp_warp, "P3\n");
		fprintf(fp_warp, "%u %u\n", ix_max_px, iy_max_px);
		fprintf(fp_warp, "%u\n", iD_max_km);	// 2^16 = 65536

		for(uint16_t iy_px = 0; iy_px < iy_max_px; iy_px++){
			for(uint16_t ix_px = 0; ix_px < ix_max_px; ix_px++){
				size_t i = iy_px * ix_max_px + ix_px;
				uint16_t iu_km = warp_u[i] + 0.5;
				uint16_t iv_km = warp_v[i] + 0.5;
				uint16_t inodata = 0;

				if(iu_km == 0 && iv_km == 0){
					inodata = iD_max_km;
				}

				fprintf(fp_warp, "%u %u %u ", iu_km, iv_km, inodata);
			}

			fprintf(fp_warp, "\n");
		}

		fclose(fp_warp);
	}



	// 地図画像読込
	char *file_r = "map.ppm";
	FILE *fp_r = fopen(file_r, "r");

	if(fp_r == NULL){
		printf("oops! R %s\n", file_r);
		exit(EXIT_FAILURE);
	}

	uint32_t imap_w = 0;	//= 6944;
	uint32_t imap_h = 0;	//= 4432;
	uint8_t imap_level_max = 0;	//= 255

	char buff[256];
	fgets(buff, 256, fp_r);	// P3

	while(1){
		fgets(buff, 256, fp_r);

		if(buff[0] == '#'){
			continue;
		}

		if(imap_w == 0){
			sscanf(buff, "%lu %lu", &imap_w, &imap_h);	// 6944 4432
		}else if(imap_level_max == 0){
			int i;
			sscanf(buff, "%u", &i);	// 255
			imap_level_max = i;
			break;
		}
	}

	if(idebug & 0x01){
		printf("imap_w [px]\t: %u\n", imap_w);
		printf("imap_h [px]\t: %u\n", imap_h);
		printf("imap_level_max\t: %u\n", imap_level_max);
		fflush(stdout);
	}



	size_t imap_num = (imap_w * imap_h) + (32-(imap_w * imap_h) % 32);
	uint8_t *imap_cr = (uint8_t *)calloc(sizeof(uint8_t), imap_num);
	uint8_t *imap_cg = (uint8_t *)calloc(sizeof(uint8_t), imap_num);
	uint8_t *imap_cb = (uint8_t *)calloc(sizeof(uint8_t), imap_num);

	if(imap_cr == NULL || imap_cg == NULL || imap_cb == NULL){
		printf("oops! short of memory\n");
		exit(EXIT_FAILURE);
	}

	if(idebug & 0x01){
		printf("imap_num\t: %u\n",imap_num);
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
		printf("imap utilize\t: %u / %u = %.3lf%%\n", imap_used, imap_num, percent);
		fflush(stdout);
	}

	fclose(fp_r);



	// ここから魚眼画像作成開始

//	const uint16_t map_scale = 16;
//	const uint16_t map_scale = 8;
//	const uint16_t map_scale = 5;
//	const uint16_t map_scale = 4;
//	const uint16_t map_scale = 3;
	const uint16_t map_scale = 2.5;
//	const uint16_t map_scale = 2;
//	const double   map_scale = 1.5;
//	const uint16_t map_scale = 1;
//	const double   map_scale = 0.8;
//	const double   map_scale = 0.5;

//	int16_t imap_u_offset_km = +9360 + 100;
//	int16_t imap_v_offset_km = -360;
//	int16_t imap_u_offset_km = 0;
//	int16_t imap_v_offset_km = -300;

	int16_t imap_u_offset_km = +100;
	int16_t imap_v_offset_km = +100;
	int16_t ifile_cnt = 0;

	system("del fish_???.ppm >nul 2>nul");
	system("del fish_???.png >nul 2>nul");

	// 北海道
//	imap_u_offset_km = +300;
//	imap_v_offset_km = -200;

	// 九州
	imap_u_offset_km = -300;
	imap_v_offset_km = +400;

	int16_t imap_du_offset_km = ((+300) - (-300)) / 600;
	int16_t imap_dv_offset_km = ((-200) - (+400)) / 600;

	while(1){
		char file_w[256] = "fish_000.ppm";
		sprintf(file_w, "fish_%03d.ppm", ifile_cnt);

		if(idebug & 0x01){
			printf("file\t: %s (%+04d %+04d)\n", file_w, imap_u_offset_km, imap_v_offset_km);
			fflush(stdout);
		}

		// 出力画像
		FILE *fp_w = fopen(file_w, "w");

		if(fp_w == NULL){
			printf("oops! W %s\n", file_w);
			exit(EXIT_FAILURE);
		}

		fprintf(fp_w, "P3\n");
		fprintf(fp_w, "%u %u\n", ix_max_px*2, iy_max_px*2);	// W H
		fprintf(fp_w, "255\n");

		uint8_t icr;
		uint8_t icg;
		uint8_t icb;



		// 上半分
		for(uint16_t iy_px = 1; iy_px <= iy_max_px; iy_px++){

			// 左上
			for(uint16_t ix_px = 1; ix_px <= ix_max_px; ix_px++){
				unsigned ix = ix_max_px - ix_px;	// 左
				unsigned iy = iy_max_px - iy_px;	// 上

				size_t i = iy * ix_max_px + ix;

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
			for(uint16_t ix_px = ix_max_px; ix_px < 2 * ix_max_px; ix_px++){
				unsigned ix = ix_px - ix_max_px;	// 右
				unsigned iy = iy_max_px - iy_px;	// 上

				size_t i = iy * ix_max_px + ix;

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

			fprintf(fp_w, "\n");	// 右端で改行
		}



		// 下半分
		for(uint16_t iy_px = iy_max_px; iy_px < 2 * iy_max_px; iy_px++){

			// 左下
			for(uint16_t ix_px = 1; ix_px <= ix_max_px; ix_px++){
				unsigned ix = ix_max_px - ix_px;	// 左
				unsigned iy = iy_px - iy_max_px;	// 下

				size_t i = iy * ix_max_px + ix;

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
			for(uint16_t ix_px = ix_max_px; ix_px < 2 * ix_max_px; ix_px++){
				unsigned ix = ix_px - ix_max_px;	// 右
				unsigned iy = iy_px - iy_max_px;	// 下

				size_t i = iy * ix_max_px + ix;

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

			fprintf(fp_w, "\n");	// 右端で改行
		}



		fclose(fp_w);



		char cmd[256];
		sprintf(cmd, "magick.exe convert %s %s", file_w, file_w);
		strcpy(cmd+strlen(cmd)-3, "png");
		system(cmd);



		if(imap_du_offset_km == 0 && imap_dv_offset_km == 0){
			break;
		}

		if(300 <= imap_u_offset_km){
			break;
		}

		imap_u_offset_km += imap_du_offset_km;
		imap_v_offset_km += imap_dv_offset_km;
		ifile_cnt++;
	}while(0);
}
