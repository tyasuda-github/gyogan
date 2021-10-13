//
// -*- coding: utf-8 -*-
// $Date: 2021/10/13 21:50:24 $
//

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

// 単位は m と rad とする。

void main(void)
{
	// デバッグ
	const uint8_t idebug = 0x08;

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

	// 地平線
	double h_max = 18.402e-3;

	// 画素サイズ的なもの
	const double h_eps = 10e-6;

	// センサーサイズの第一象限サイズ
	const uint16_t ix_max_um = (W/2/h_eps) + 0.5;
	const uint16_t iy_max_um = (H/2/h_eps) + 0.5;

	// 変換テーブル
	const size_t iconv_num = (ix_max_um+1) * (iy_max_um+1) + 0.5;
	if(idebug & 0x01){ printf("%+8u\n", iconv_num); }
	uint16_t *iconv_u = (uint16_t *)calloc(sizeof(uint16_t), iconv_num);
	uint16_t *iconv_v = (uint16_t *)calloc(sizeof(uint16_t), iconv_num);

	if(iconv_u == NULL || iconv_v == NULL){
		printf("oops! short of memory\n");
		exit(EXIT_FAILURE);
	}

	// 地上距離の刻み幅
	const double alpha = ix_max_um / (ix_max_um + 1.0);

	// 地上距離の初期値
	double d = d_max;

	// 直前の像高計算結果
	double h_um_last = 0;

	// テーブル使用率
	size_t iconv_used = 0;

	while(1){
		// 中間変数
		double phi = d / R;

		// 魚眼入射角
		double theta = atan(sin(phi) / ((a/R+1) - cos(phi)));

		// 像高
		double h = 2 * f * sin(theta / 2);

		double d_km = d / 1000;
		double h_um = h / h_eps;

		if((h_um_last - 0.75 < h_um && h_um < h_um_last - 0.25) || h_um_last == 0){
			if(h_um_last == 0){
				h_max = h;
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
					if(iconv_num <= i){ printf("oops! A %+8u (%+4u %+4u)um\n", i, ix_um, iy_um); exit(EXIT_FAILURE); }
					if(idebug & 0x04){ printf("  %+8u A (%+4u %+4u)um (%+4u %+4u)km\n", i, ix_um, iy_um, iu_km, iv_km); }
					iconv_u[i] = iu_km;
					iconv_v[i] = iv_km;
					iconv_used++;
				}

				if(ix_um <= iy_max_um && iy_um <= ix_max_um){
					size_t i = ix_um * ix_max_um + iy_um;
					if(iconv_num <= i){ printf("oops! iB %+8u (%+4u %+4u)um\n", i, iy_um, ix_um); exit(EXIT_FAILURE); }
					if(idebug & 0x04){ printf("  %+8u iB (%+4u %+4u)um (%+4u %+4u)km\n", i, iy_um, ix_um, iv_km, iu_km); }
					iconv_u[i] = iv_km;
					iconv_v[i] = iu_km;
					iconv_used++;
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

	if(idebug & 0x08){ printf("%u / %u\t%+8.3lf%%\n", iconv_used, iconv_num, 100.0*iconv_used/iconv_num); }

	//-------------------------------------------------------------------------

	// 地図画像
	char *file_r = "map.ppm";
	FILE *fp_r = fopen(file_r, "r");
	if(fp_r == NULL){ printf("oops! R %s\n", file_r); exit(EXIT_FAILURE); }
	const uint16_t imap_w = 1600;
	const uint16_t imap_h = 1600;

	uint8_t *imap_cr = (uint8_t *)calloc(sizeof(uint8_t), imap_w * imap_h);
	uint8_t *imap_cg = (uint8_t *)calloc(sizeof(uint8_t), imap_w * imap_h);
	uint8_t *imap_cb = (uint8_t *)calloc(sizeof(uint8_t), imap_w * imap_h);

	if(imap_cr == NULL || imap_cg == NULL || imap_cb == NULL){
		printf("oops! short of memory\n");
		exit(EXIT_FAILURE);
	}

	char buff[256];
	fgets(buff, 256, fp_r);	// P3
	fgets(buff, 256, fp_r);	// 1600 1600
	fgets(buff, 256, fp_r);	// 255
	size_t i = 0, ix = 0, iy = 0;

	while(feof(fp_r) == 0){
		int icr, icg, icb;
		fscanf(fp_r, "%d%d%d", &icr, &icg, &icb);
		if(idebug & 0x10){ printf("(%8u %4u %4u) (%4d %4d %4d)\n", i, ix, iy, icr, icg, icb); };

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

	// 出力画像
	char *file_w = "fish.ppm";
	FILE *fp_w = fopen(file_w, "w");
	if(fp_w == NULL){ printf("oops! W %s\n", file_w); exit(EXIT_FAILURE); }

	const size_t ifish_num = (2 * ix_max_um + 1) * (2 * iy_max_um + 1);
	if(idebug & 0x01){ printf("%+8u\n", ifish_num); }
	uint8_t *fish_x = (uint8_t *)calloc(sizeof(uint8_t), ifish_num);
	uint8_t *fish_y = (uint8_t *)calloc(sizeof(uint8_t), ifish_num);

	if(iconv_u == NULL || iconv_v == NULL){
		printf("oops! short of memory\n");
		exit(EXIT_FAILURE);
	}

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
			uint16_t iu_km = iconv_u[i];
			uint16_t iv_km = iconv_v[i];

			if(iu_km == 0 && iv_km == 0){
				fprintf(fp_w, "%u %u %u ", 0, 0, 0);
			}else{
				size_t imap_u = imap_w / 2 - iu_km / 16;	// 左
				size_t imap_v = imap_h / 2 - iv_km / 16;	// 上
				size_t i = imap_v * imap_h + imap_u;

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
			uint16_t iu_km = iconv_u[i];
			uint16_t iv_km = iconv_v[i];

			if(iu_km == 0 && iv_km == 0){
				fprintf(fp_w, "%u %u %u ", 0, 0, 0);
			}else{
				size_t imap_u = iu_km / 16 + imap_w / 2;	// 右
				size_t imap_v = imap_h / 2 - iv_km / 16;	// 上
				size_t i = imap_v * imap_h + imap_u;

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
			uint16_t iu_km = iconv_u[i];
			uint16_t iv_km = iconv_v[i];

			if(iu_km == 0 && iv_km == 0){
				fprintf(fp_w, "%u %u %u ", 0, 0, 0);
			}else{
				size_t imap_u = imap_w / 2 - iu_km / 16;	// 左
				size_t imap_v = iv_km / 16 + imap_h / 2;	// 下
				size_t i = imap_v * imap_h + imap_u;

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
			uint16_t iu_km = iconv_u[i];
			uint16_t iv_km = iconv_v[i];

			if(iu_km == 0 && iv_km == 0){
				fprintf(fp_w, "%u %u %u ", 0, 0, 0);
			}else{
				size_t imap_u = iu_km / 16 + imap_w / 2;	// 右
				size_t imap_v = iv_km / 16 + imap_h / 2;	// 下
				size_t i = imap_v * imap_h + imap_u;

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
}
