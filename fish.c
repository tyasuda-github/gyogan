//
// -*- coding: utf-8 -*-
// $Date: 2021/10/14 12:04:58 $
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

// 単位は m と rad とする。

int main(int argc, char *argv[])
{
	// デバッグ
//	const uint8_t idebug = (2 <= argc) ? (uint8_t)atof(argv[1]) : 0x00;
	const uint8_t idebug = 0x01;

	if(idebug){
		printf("%s\n", argv[0]);
		printf("idebug(0x%02X)\n", idebug);
	}

	// 魚眼の焦点距離
	double f = 16e-3;

	// 衛星の高度
	const double a = 400e3;

	// 地球の半径
	const double R = 6371e3;

	// センサーサイズ
	const double W = 36e-3;
	const double H = 24e-3;

	// 地平線
	const double theta_max = asin(R / (a+R));
	const double d_max = R * (M_PI / 2 - theta_max);
	double h_max = 18.402;

	// 画素サイズ的なもの
	const double h_eps = 10e-6;

	// センサーサイズの第一象限サイズ
	const uint16_t ix_max_um = (W/2/h_eps) + 0.5;
	const uint16_t iy_max_um = (H/2/h_eps) + 0.5;

	// 地上距離の刻み幅
	const double alpha = ix_max_um / (ix_max_um + 1.0);

	//-------------------------------------------------------------------------

	// ワープマップ
	const size_t iwarp_num = (ix_max_um+1) * (iy_max_um+1);
	uint16_t *iwarp_u = (uint16_t *)calloc(sizeof(uint16_t), iwarp_num);
	uint16_t *iwarp_v = (uint16_t *)calloc(sizeof(uint16_t), iwarp_num);

	if(iwarp_u == NULL || iwarp_v == NULL){
		printf("oops! short of memory\n");
		exit(EXIT_FAILURE);
	}

	if(idebug & 0x01){
		const double d_max_km = d_max / 1000;
		printf("d_max_km(%+8.3lf)\n", d_max_km);
		printf("iwarp_num(%u)\n", iwarp_num);
	}

	//-------------------------------------------------------------------------

	// ワープマップ読込
	size_t iwarp_used = 0;
	const char *file_warp = "warp_map.ppm";
	FILE *fp_warp = fopen(file_warp, "r");

	if(fp_warp == NULL){

	}else{
		char buff[256];
		uint16_t iwarp_w, iwarp_h;
		fgets(buff, 256, fp_warp);						// P3
		fscanf(fp_warp, "%lu %lu", &iwarp_w, &iwarp_h);	// 6944 4432
		fscanf(fp_warp, "%s", buff);					// 255

		if(idebug & 0x01){
			printf("iwarp_w(%u)\n", iwarp_w);
			printf("iwarp_h(%u)\n", iwarp_h);
			printf("iwarp_level_max(%s)\n", buff);
			printf("iwarp_num(%u)\n",iwarp_num);
		}

		size_t i = 0, ix = 0, iy = 0;

		while(feof(fp_warp) == 0){
			int iu_km, iv_km, inodata;

			if(fscanf(fp_warp, "%d %d %d", &iu_km, &iv_km, &inodata) != 3){
				break;
			}

			if(idebug & 0x08){
				printf("(%8u %4u %4u) (%4d %4d %4d)\n", i, ix, iy, iu_km, iv_km, inodata);
			}

			iwarp_u[i] = (uint16_t)iu_km;
			iwarp_v[i] = (uint16_t)iv_km;

			if(ix == iwarp_w-1){
				ix = 0;
				iy++;
			}else{
				ix++;
			}

			i++;
		}

		iwarp_used = i;
		fclose(fp_warp);
	}

	//-------------------------------------------------------------------------

	// ワープマップ作成

	// 地上距離の初期値
	double d = d_max;

	// 直前の像高計算結果
	double h_um_last = 0;

	while(fp_warp){
		// 中間変数
		double phi = d / R;

		// 魚眼入射角
		double theta = atan(sin(phi) / ((a/R+1) - cos(phi)));

		// 像高
		double h = 2 * f * sin(theta / 2);

		double d_km = d / 1000;
		double h_um = h / h_eps;

		if((h_um_last - 0.75 < h_um && h_um < h_um_last - 0.25) || h_um_last == 0){
			if(h && h_max == 0){
				h_max = h;
				double theta_max_deg = theta / M_PI * 180;

				if(idebug & 0x01){
					double h_max_mm = h_max * 1000;
					printf("theta_max_deg(%+.3lf)\n", theta_max_deg);
					printf("h_max_mm(%+.3lf)\n", h_max_mm);
				}
			}

			if(idebug & 0x02){
				double theta_deg = theta / M_PI * 180;
				double h_mm = h * 1000;
				printf("%+8.3lfmm\t%+8.3lfkm\t%+8.3lfo\n", h_mm, d_km, theta_deg);
			}

			h_um_last = h_um;



			uint16_t ih_um = h_um + 0.5;
			uint16_t ix_or_h_max_um = (ix_max_um < ih_um) ? ix_max_um : ih_um;

			for(uint16_t ix_um = 0; ix_um <= ix_or_h_max_um; ix_um++){
				uint16_t iy_um = sqrt(h_um * h_um - ix_um * ix_um) + 0.5;

				// 45°線に対して対称な点は省略
				if(iy_um < ix_um){
					break;
				}

				uint16_t iu_km = (ix_um * h_eps / h * d_km) + 0.5;
				uint16_t iv_km = (iy_um * h_eps / h * d_km) + 0.5;

				if(iy_um <= iy_max_um){
					size_t i = iy_um * ix_max_um + ix_um;
					if(iwarp_num <= i){ printf("oops! A %+8u (%+4u %+4u)um\n", i, ix_um, iy_um); exit(EXIT_FAILURE); }
					if(idebug & 0x04){ printf("  %+8u A (%+4u %+4u)um (%+4u %+4u)km\n", i, ix_um, iy_um, iu_km, iv_km); }
					iwarp_u[i] = iu_km;
					iwarp_v[i] = iv_km;
					iwarp_used++;
				}

				if(ix_um <= iy_max_um && iy_um <= ix_max_um){
					size_t i = ix_um * ix_max_um + iy_um;
					if(iwarp_num <= i){ printf("oops! iB %+8u (%+4u %+4u)um\n", i, iy_um, ix_um); exit(EXIT_FAILURE); }
					if(idebug & 0x04){ printf("  %+8u iB (%+4u %+4u)um (%+4u %+4u)km\n", i, iy_um, ix_um, iv_km, iu_km); }
					iwarp_u[i] = iv_km;
					iwarp_v[i] = iu_km;
					iwarp_used++;
				}
			}



		}else if(h_um <= h_um_last - 0.75){
			// 少し戻る
			d = (d + d / alpha) / 2;
			continue;
		}

		// 適当に地上の距離を短くしてみる
		d *= alpha;

		// 像高が画素サイズよりも小さくなったら終了
		if(h < h_eps){
			break;
		}
	}

	//-------------------------------------------------------------------------

	if(idebug & 0x01){
		double percent = 100.0*iwarp_used/iwarp_num;
		printf("iwarp: %u / %u = %.3lf%%\n", iwarp_used, iwarp_num, percent);
	}

	//-------------------------------------------------------------------------

	// ワープマップ保存
	if(fp_warp == NULL){
		fp_warp = fopen("warp_map_verify.ppm", "w");
		if(fp_warp == NULL){ printf("oops! W %s\n", file_warp); exit(EXIT_FAILURE); }

		// 地球の円周
		const uint16_t id_max_km = d_max / 1000 + 0.5;

		fprintf(fp_warp, "P3\n");
		fprintf(fp_warp, "%u %u\n", ix_max_um, iy_max_um);
		fprintf(fp_warp, "%u\n", id_max_km);	// 2^16 = 65536

		for(uint16_t iy_um = 0; iy_um < iy_max_um; iy_um++){
			for(uint16_t ix_um = 0; ix_um < ix_max_um; ix_um++){
				size_t i = iy_um * ix_max_um + ix_um;
				uint16_t ux_km = iwarp_u[i];
				uint16_t uy_km = iwarp_v[i];
				int no_data = (ux_km == 0 && uy_km == 0) ? id_max_km : 0;
				fprintf(fp_warp, "%u %u %u ", ux_km, uy_km, no_data);
			}

			fprintf(fp_warp, "\n");
		}

		fclose(fp_warp);
	}

	//-------------------------------------------------------------------------

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

	const uint16_t imap_scale = 5;
//	const uint16_t imap_scale = 2;
//	const double imap_scale = 1;

	if(idebug & 0x01){
		printf("imap_w(%u)\n", imap_w);
		printf("imap_h(%u)\n", imap_h);
		printf("imap_level_max(%s)\n",buff);
		printf("imap_num(%u)\n",imap_num);
	}

	size_t i = 0, ix = 0, iy = 0;

	while(feof(fp_r) == 0){
		int icr, icg, icb;

		if(fscanf(fp_r, "%d %d %d", &icr, &icg, &icb) != 3){
			break;
		}

		if(idebug & 0x10){
			printf("(%8u %4u %4u) (%4d %4d %4d)\n", i, ix, iy, icr, icg, icb);
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

	int16_t imap_u_offset_km = +9360 + 100;
	int16_t imap_v_offset_km = -360;

	do{
	char filename[256] = "fish_0000.ppm";
	sprintf(filename, "fish_%04d.ppm", imap_u_offset_km);

	if(idebug & 0x01){
		printf("imap_u/v_offset_km(%+d %+d)\n", imap_u_offset_km, imap_v_offset_km);
		fflush(stdout);
	}

	// 出力画像
	char *file_w = filename;
	FILE *fp_w = fopen(file_w, "w");
	if(fp_w == NULL){ printf("oops! W %s\n", file_w); exit(EXIT_FAILURE); }

	fputs("P3\n", fp_w);
	fputs("3600 2400\n", fp_w);	// W H
	fputs("255\n", fp_w);

	uint8_t icr;
	uint8_t icg;
	uint8_t icb;
	uint8_t ilast_data_valid = 0;

	// 上半分
	for(uint16_t iy_um = 0; iy_um < iy_max_um; iy_um++){
		ilast_data_valid = 0;

		// 左上
		for(uint16_t ix_um = 0; ix_um < ix_max_um; ix_um++){
			unsigned ix = ix_max_um - ix_um;	// 左
			unsigned iy = iy_max_um - iy_um;	// 上

			size_t i = iy * ix_max_um + ix;
			uint16_t iu_km = iwarp_u[i];
			uint16_t iv_km = iwarp_v[i];

			if(iu_km == 0 && iv_km == 0){
				fprintf(fp_w, "%u %u %u ", 0, 0, 0);
			}else{
				size_t imap_u = imap_w / 2 - iu_km / imap_scale + imap_u_offset_km;	// 左
				size_t imap_v = imap_h / 2 - iv_km / imap_scale + imap_v_offset_km;	// 上
				size_t i = imap_v * imap_w + imap_u;

				if(imap_num <= i){
					printf("imap_u/v(%u %u) i(%u) imap_num(%u)\n", imap_u, imap_v, i, imap_num);
					exit(EXIT_FAILURE);
				}

				icr = imap_cr[i];
				icg = imap_cg[i];
				icb = imap_cb[i];

//				fprintf(fp_w, "%u %u %u ", imap_u % 256, imap_v % 256, 0);	// R G B
				fprintf(fp_w, "%u %u %u ", icr, icg, icb);	// R G B
//-				ilast_data_valid++;
			}
		}

		// 右上
		for(uint16_t ix_um = ix_max_um; ix_um < 2 * ix_max_um; ix_um++){
			unsigned ix = ix_um - ix_max_um;	// 右
			unsigned iy = iy_max_um - iy_um;	// 上

			size_t i = iy * ix_max_um + ix;
			uint16_t iu_km = iwarp_u[i];
			uint16_t iv_km = iwarp_v[i];

			if(iu_km == 0 && iv_km == 0){
				fprintf(fp_w, "%u %u %u ", 0, 0, 0);
			}else{
				size_t imap_u = iu_km / imap_scale + imap_w / 2 + imap_u_offset_km;	// 右
				size_t imap_v = imap_h / 2 - iv_km / imap_scale + imap_v_offset_km;	// 上
				size_t i = imap_v * imap_w + imap_u;

				if(imap_num <= i){
					printf("imap_u/v(%u %u) i(%u) imap_num(%u)\n", imap_u, imap_v, i, imap_num);
					exit(EXIT_FAILURE);
				}

				icr = imap_cr[i];
				icg = imap_cg[i];
				icb = imap_cb[i];

//				fprintf(fp_w, "%u %u %u ", imap_u % 256, imap_v % 256, 0);	// R G B
				fprintf(fp_w, "%u %u %u ", icr, icg, icb);	// R G B
//-				ilast_data_valid++;
			}
		}

		fprintf(fp_w, "\n");
	}

	// 下半分
	for(uint16_t iy_um = iy_max_um; iy_um < 2 * iy_max_um; iy_um++){
		ilast_data_valid = 0;

		// 左下
		for(uint16_t ix_um = 0; ix_um < ix_max_um; ix_um++){
			unsigned ix = ix_max_um - ix_um;	// 左
			unsigned iy = iy_um - iy_max_um;	// 下

			size_t i = iy * ix_max_um + ix;
			uint16_t iu_km = iwarp_u[i];
			uint16_t iv_km = iwarp_v[i];

			if(iu_km == 0 && iv_km == 0){
				fprintf(fp_w, "%u %u %u ", 0, 0, 0);
			}else{
				size_t imap_u = imap_w / 2 - iu_km / imap_scale + imap_u_offset_km;	// 左
				size_t imap_v = iv_km / imap_scale + imap_h / 2 + imap_v_offset_km;	// 下
				size_t i = imap_v * imap_w + imap_u;

				if(imap_num <= i){
					printf("imap_u/v(%u %u) i(%u) imap_num(%u)\n", imap_u, imap_v, i, imap_num);
					exit(EXIT_FAILURE);
				}

				icr = imap_cr[i];
				icg = imap_cg[i];
				icb = imap_cb[i];

//				fprintf(fp_w, "%u %u %u ", imap_u % 256, imap_v % 256, 0);	// R G B
				fprintf(fp_w, "%u %u %u ", icr, icg, icb);	// R G B
//-				ilast_data_valid++;
			}
		}

		// 右下
		for(uint16_t ix_um = ix_max_um; ix_um < 2 * ix_max_um; ix_um++){
			unsigned ix = ix_um - ix_max_um;	// 右
			unsigned iy = iy_um - iy_max_um;	// 下

			size_t i = iy * ix_max_um + ix;
			uint16_t iu_km = iwarp_u[i];
			uint16_t iv_km = iwarp_v[i];

			if(iu_km == 0 && iv_km == 0){
				fprintf(fp_w, "%u %u %u ", 0, 0, 0);
			}else{
				size_t imap_u = iu_km / imap_scale + imap_w / 2 + imap_u_offset_km;	// 右
				size_t imap_v = iv_km / imap_scale + imap_h / 2 + imap_v_offset_km;	// 下
				size_t i = imap_v * imap_w + imap_u;

				if(imap_num <= i){
					printf("imap_u/v(%u %u) i(%u) imap_num(%u)\n", imap_u, imap_v, i, imap_num);
					exit(EXIT_FAILURE);
				}

				icr = imap_cr[i];
				icg = imap_cg[i];
				icb = imap_cb[i];

//				fprintf(fp_w, "%u %u %u ", imap_u % 256, imap_v % 256, 0);	// R G B
				fprintf(fp_w, "%u %u %u ", icr, icg, icb);	// R G B
//-				ilast_data_valid++;
			}
		}

		fprintf(fp_w, "\n");
	}

	fclose(fp_w);
	sprintf(buff, "copy %s fish.ppm >nul", filename);
	system(buff);

	imap_u_offset_km -= 1;
	}while(9360 - 100 <= imap_u_offset_km);
}
