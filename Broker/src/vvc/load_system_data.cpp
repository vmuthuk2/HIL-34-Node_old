//# define ARMA_DONT_USE_WRAPPER

#include <armadillo>
#include "load_system_data.h"
//using namespace arma;

sysdata load_system_data()
{
	sysdata data1;
	
	data1.Rpv = 1;
	double Rpv = data1.Rpv;
	data1.Rsst = 0;
	double Rsst = data1.Rsst;
	data1.Rcap = 0;
	double Rcap = data1.Rcap;
	
	data1.bkva = 10000;
	data1.bkv = 24.9;
	data1.vo = 24.9*1.05;
	data1.eps = 0.000001;
	data1.mxitr = 100;
	data1.delta_Ploss = 0;
	data1.Ploss_mis_a = 1;
	data1.Qlimit = 0.2; //initial limitation, 10kVar
	data1.itea_Qlimit = 1;
	data1.lb_v = 0.85;  //lower voltage limit
	data1.ub_v = 1.10;  //upper voltage limit
	data1.Ploss_mis_per_dP = 1;
	data1.Qr = 0.6;

	
	//		    ln	   sbus	   ldbus   lcod    	lng		    ldty	  P1          Q1		P2			Q2		    P3			 Q3           QC
data1.Dl << 1 << 0 << 1 << 1 << 2580   << 1 << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 2 << 1 << 2 << 1 << 1730   << 1 << 0*Rpv << 0*Rsst << 30*Rpv << 15*Rsst << 25*Rpv << 14*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 3 << 2 << 3 << 1 << 32230  << 1 << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 4 << 3 << 4 << 1 << 37500  << 1 << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 5 << 4 << 5 << 1 << 29730  << 1 << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 6 << 5 << 6 << 8 << 1 << 1 << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 1.05 << 1.05 << 1.05 << arma::endr
	 << 7 << 6 << 7 << 2 << 310    << 1 << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 8 << 7 << 8 << 2 << 10210  << 1 << 0*Rpv << 0*Rsst << 5*Rpv << 2*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 9 << 8 << 9 << 2 << 840    << 1 << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 4*Rpv << 2*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 10 << 9 << 10 << 2 << 20440<< 1 << 17*Rpv << 8*Rsst << 10*Rpv << 5*Rsst << 25*Rpv << 10*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 11 << 10 << 11 << 2 << 520 << 1 << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 12 << 11 << 12 << 2 << 36830<< 1 << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 13 << 12 << 13 << 2 << 10  << 1 << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
 	 << 14 << 13 << 14 << 2 << 4900<< 1 << 7*Rpv << 3*Rsst << 2*Rpv << 1*Rsst << 6*Rpv << 3*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 15 << 14 << 15 << 2 << 5830<< 1 << 4*Rpv << 2*Rsst << 15*Rpv << 8*Rsst << 13*Rpv << 7*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 16 << 15 << 16 << 2 << 2020<< 1 << 36*Rpv<< 24*Rsst<< 40*Rpv << 26*Rsst << 130*Rpv << 71*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 17 << 16 << 17 << 2 << 2680<< 1 << 30*Rpv<< 15*Rsst<< 10*Rpv << 6*Rsst << 42*Rpv << 22*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 18 << 17 << 18 << 2 << 280 << 1 << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 19 << 18 << 19 << 5 << 4860<< 1 << 0*Rpv << 0*Rsst << 28*Rpv << 14*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 0  << 0  << 0  << 0 << 0   << 0 << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 0 << 0 << 0 << arma::endr
	 << 20 << 3 << 20 << 4 << 5804 << 1 << 0*Rpv << 0*Rsst << 16*Rpv << 8*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 0  << 0  << 0  << 0 << 0   << 0 << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 0 << 0 << 0 << arma::endr
	 << 21 << 7 << 21 << 3 << 1710 << 1 << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 22 << 21 << 22 << 3 << 48150<< 1 << 34*Rpv << 17*Rsst << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 23 << 22 << 23 << 3 << 13740<< 1 << 135*Rpv << 70*Rsst << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 0  << 0  << 0  << 0 << 0 	<< 0 << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 0 << 0 << 0 << arma::endr
	 << 24 << 8 << 24 << 4 << 3030 << 1 << 0*Rpv << 0*Rsst << 40*Rpv << 20*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 0  << 0 << 0  << 0 << 0    << 0 << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 0 << 0 << 0 << arma::endr
	 << 25 << 11 << 25 << 4 << 23330<< 1 << 0*Rpv << 0*Rsst << 4*Rpv << 2*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 0  << 0  << 0 << 0 << 0    << 0 << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 0 << 0 << 0 << arma::endr
	 << 26 << 13 << 26 << 1 << 10  << 1 << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 27 << 26 << 27 << 1 << 10560<< 1 << 150*Rpv << 75*Rsst << 150*Rpv << 75*Rsst << 150*Rpv << 75*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 0  << 0  << 0  << 0 << 0   << 0 << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 0 << 0 << 0 << arma::endr
	 << 28 << 14 << 28 << 3 << 1620<< 1 << 2*Rpv << 1*Rsst << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 0  << 0  << 0  << 0 << 0   << 0 << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 0 << 0 << 0 << arma::endr
	 << 29 << 15 << 29 << 2 << 280 << 1 << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 30 << 29 << 30 << 2 << 1350<< 1 << 144*Rpv << 110*Rsst << 135*Rpv << 105*Rsst << 135*Rpv << 105*Rsst << 300*Rcap << 1 << 1 << 1 << arma::endr
	 << 31 << 30 << 31 << 2 << 3640<< 1 << 0*Rpv << 0*Rsst << 25 * Rpv << 12*Rsst << 20*Rpv << 11*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr
	 << 32 << 31 << 32 << 2 << 530 << 1 << 20*Rpv << 16*Rsst << 43*Rpv << 27*Rsst << 20*Rpv << 16*Rsst << 450*Rcap << 1 << 1 << 1 << arma::endr
	 << 0  << 0  << 0  << 0 << 0   << 0 << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rpv << 0*Rsst << 0*Rcap << 0 << 0 << 0 << arma::endr
	 << 33 << 17 << 33 << 2 << 860 << 1 << 27*Rpv << 16*Rsst << 31*Rpv << 18*Rsst << 9*Rpv << 7*Rsst << 0*Rcap << 1 << 1 << 1 << arma::endr;


		arma::mat R, X;//in ohms
	      R << 1.3668 << 0.2101 << 0.2130 << arma::endr
		<< 0.2101 << 1.3238 << 0.2066 << arma::endr
		<< 0.2130 << 0.2066 << 1.3294 << arma::endr// 300
		<< 1.9300 << 0.2327 << 0.2359 << arma::endr
		<< 0.2327 << 1.9157 << 0.2288 << arma::endr
		<< 0.2359 << 0.2288 << 1.9219 << arma::endr// 301
		<< 2.7995 << 0 << 0 << arma::endr
		<< 0 << 0 << 0 << arma::endr
		<< 0 << 0 << 0 << arma::endr// 302
		<< 0 << 0 << 0 << arma::endr
		<< 0 << 2.7995 << 0 << arma::endr
		<< 0 << 0 << 0 << arma::endr// 303
		<< 0 << 0 << 0 << arma::endr
		<< 0 << 1.9217 << 0 << arma::endr
		<< 0 << 0 << 0 << arma::endr// 304
		<< 0.0369 << 0.0058 << 0.0057 << arma::endr
		<< 0.0058 << 0.0373 << 0.0059 << arma::endr
		<< 0.0057 << 0.0059 << 0.0370 << arma::endr// 200
		<< 0.0190 << 0 << 0 << arma::endr
		<< 0 << 0.0190 << 0 << arma::endr
		<< 0 << 0 << 0.0190 << arma::endr// Tx
		<< 0 << 0 << 0 << arma::endr
		<< 0 << 0 << 0 << arma::endr
		<< 0 << 0 << 0 << arma::endr;// VR



	      X << 1.3343 << 0.5779 << 0.5015 << arma::endr
		<< 0.5779 << 1.3569 << 0.4591 << arma::endr
		<< 0.5015 << 0.4591 << 1.3471 << arma::endr// 300
		<< 1.4115 << 0.6442 << 0.5691 << arma::endr
		<< 0.6442 << 1.4281 << 0.5238 << arma::endr
		<< 0.5691 << 0.5238 << 1.4209 << arma::endr// 301
		<< 1.4855 << 0 << 0 << arma::endr
		<< 0 << 0 << 0 << arma::endr
		<< 0 << 0 << 0 << arma::endr// 302
		<< 0 << 0 << 0 << arma::endr
		<< 0 << 1.4855 << 0 << arma::endr
		<< 0 << 0 << 0 << arma::endr// 303
		<< 0 << 0 << 0 << arma::endr
		<< 0 << 1.4212 << 0 << arma::endr
		<< 0 << 0 << 0 << arma::endr// 304
		<< 0.0378 << 0.0166 << 0.0128 << arma::endr
		<< 0.0166 << 0.0371 << 0.0140 << arma::endr
		<< 0.0128 << 0.0140 << 0.0377 << arma::endr// 200
		<< 0.0408 << 0 << 0 << arma::endr
		<< 0 << 0.0408 << 0 << arma::endr
		<< 0 << 0 << 0.0408 << arma::endr// Tx
		<< 0.0500 << 0 << 0 << arma::endr
		<< 0 << 0.0500 << 0 << arma::endr
		<< 0 << 0 << 0.0500 << arma::endr;// VR
		
	data1.Z = arma::cx_mat(R, X);



	return data1;
};
