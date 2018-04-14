//# define ARMA_DONT_USE_WRAPPER

#include "armadillo"
#include "fun_return.h"
#include "cmath"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;
using namespace arma;

y_re form_Y_abc(arma::mat Dl, arma::cx_mat Z, double bkva, double bkv)
{
	y_re y_results;
	double Zb = pow(bkv, 2) / bkva * 1000;
	
		
	Dl.col(4) = Dl.col(4)/5280; //added by valli 12/24/2017

	int Ldl = Dl.n_rows;//rows of Dl matrix

	arma::uvec x = find(Dl.col(0) > 0);
	int Lbr = x.n_rows;// branchNo

	//cout << "Lbr: No of Branches=" << Lbr << endl;
	y_results.brnches = arma::cx_mat(arma::zeros(Lbr, 8), arma::zeros(Lbr, 8));

	int j = 0;

	for (int i = 0; i < Ldl && j < Lbr; ++i)
	{
		int idx = 3 * ((int)Dl(i, 3) - 1);
		if ((int)Dl(i, 0) != 0)
		{
			y_results.brnches(j, 0) = Dl(i, 1);
			y_results.brnches(j, 1) = Dl(i, 2);
			if ((int)Dl(i, 3) == 7)
			{
				y_results.brnches(j, 2) = Z(idx, 0);
				y_results.brnches(j, 3) = Z(idx + 1, 1);
				y_results.brnches(j, 4) = Z(idx + 2, 2);
			}
			if ((int)Dl(i, 3) == 8)
			{
				y_results.brnches(j, 2) = Z(idx, 0);
				y_results.brnches(j, 3) = Z(idx + 1, 1);
				y_results.brnches(j, 4) = Z(idx + 2, 2);
			}
			else{
				y_results.brnches(j, 2) = Dl(i, 4)*Z(idx, 0) / Zb;  //self impedance of phase A, B, C in pu
				y_results.brnches(j, 3) = Dl(i, 4)*Z(idx + 1, 1) / Zb;  //self impedance of phase A, B, C in pu
				y_results.brnches(j, 4) = Dl(i, 4)*Z(idx + 2, 2) / Zb; //self impedance of phase A, B, C in pu
			}
			y_results.brnches(j, 5) = Dl(i, 13);
			y_results.brnches(j, 6) = Dl(i, 14);
			y_results.brnches(j, 7) = Dl(i, 15);
			++j;
		}
	}
	//std::cout << "brnches: " << endl << y_results.brnches << endl;
	arma::cx_mat brnches = y_results.brnches;
	//for brnch matrix for phase ABC separately

	arma::uvec x_a = find(abs(y_results.brnches.col(2)) > 0);
	int Lnum_a = x_a.n_rows;// phase A branchNo
	arma::cx_mat brnches_a = arma::cx_mat(arma::zeros(Lnum_a, 6), arma::zeros(Lnum_a, 6));
	//cout << "branches_a:" << brnches_a.n_rows << "*" << brnches_a.n_cols << endl;

	arma::uvec x_b = find(abs(y_results.brnches.col(3)) > 0);
	int Lnum_b = x_b.n_rows;// phase B branchNo
	arma::cx_mat brnches_b = arma::cx_mat(arma::zeros(Lnum_b, 6), arma::zeros(Lnum_b, 6));
	//cout << "branches_b:" << brnches_b.n_rows << "*" << brnches_b.n_cols << endl;

	arma::uvec x_c = find(abs(y_results.brnches.col(4)) > 0);
	int Lnum_c = x_c.n_rows;// phase C branchNo
	arma::cx_mat brnches_c = arma::cx_mat(arma::zeros(Lnum_c, 6), arma::zeros(Lnum_c, 6));
	//cout << "branches_c:" << brnches_c.n_rows << "*" << brnches_c.n_cols << endl;
	
	//cout << Lnum_a << Lnum_b << Lnum_c << endl;
	int ja = 0;
	int jb = 0;
	int jc = 0;
	for (int i = 0; i < Lbr && ja < Lnum_a && jb < Lnum_b && jc < Lnum_c; ++i)
	{
		if (abs(y_results.brnches(i, 2)) != 0)
		{
			brnches_a(ja, 0) = y_results.brnches(i, 0);
			brnches_a(ja, 1) = y_results.brnches(i, 1);
			brnches_a(ja, 2) = y_results.brnches(i, 2);
			brnches_a(ja, 3) = y_results.brnches(i, 5);
			++ja;
		}
		if (abs(y_results.brnches(i, 3)) != 0)
		{
			brnches_b(jb, 0) = y_results.brnches(i, 0);
			brnches_b(jb, 1) = y_results.brnches(i, 1);
			brnches_b(jb, 2) = y_results.brnches(i, 3);
			brnches_b(jb, 3) = y_results.brnches(i, 6);
			++jb;
		}
		if (abs(y_results.brnches(i, 4)) != 0)
		{
			brnches_c(jc, 0) = y_results.brnches(i, 0);
			brnches_c(jc, 1) = y_results.brnches(i, 1);
			brnches_c(jc, 2) = y_results.brnches(i, 4);
			brnches_c(jc, 3) = y_results.brnches(i, 7);
			++jc;
		}
	}

	//cout << "branches_a:" << imag(brnches_a) << endl;
	//cout << "branches_b:" << real(brnches_b) << endl;
	//cout << "branches_c:" << real(brnches_c) << endl;

	//Rename the node for each phase - added by Valli 12/24/2017
	arma::cx_mat main_a_temp = arma::cx_mat(arma::zeros(Lnum_a, 2), arma::zeros(Lnum_a, 2));
	arma::cx_mat cx_unity = arma::cx_mat(arma::ones(1, 1), arma::zeros(1, 1));
	main_a_temp(0,0) = brnches_a(0,0);
	main_a_temp(0,1) = brnches_a(0,1);
	//std::cout << main_a_temp << endl;
	int a = 1;
	for (int i = 1; i < Lbr; ++i)
	{
		if (brnches_a(i,0) == (brnches_a(i-1,0)+cx_unity(0, 0)))
		{
			main_a_temp(a,0) = brnches_a(i,0);
			main_a_temp(a,1) = brnches_a(i,1);
			a = a+1;
		}
		else
		break;
	}
	//std::cout << main_a_temp << endl;
	int Lma;
	int Wma = main_a_temp.n_cols;
	x = find(abs(main_a_temp.col(1))>0);
	Lma = x.n_rows;
	//cout << "x: " << endl << x << endl;
	//cout << Lma << Wma << endl;
	arma::cx_mat main_a = arma::cx_mat(arma::zeros(Lma, Wma), arma::zeros(Lma, Wma));
	
	main_a = main_a_temp.submat (0, 0, size(main_a));
	
	//cout << "brnches_a: " << endl << brnches_a << endl;
	arma::cx_mat lateral_a_temp = arma::cx_mat(arma::zeros(Lnum_a, 2), arma::zeros(Lnum_a, 2));
	a = 0;
	int b = Lma;
	for (int i = Lma; i < Lnum_a; ++i)
	{
		if (abs(brnches_a(i,1)) > b)
		{
			lateral_a_temp(a,1) = b+1;
			a = a+1;
			b = b+1; 
		}
		else
		{
			lateral_a_temp(a,1) = brnches_a(i,1);
			a = a+1;
		}
	}
	
	x = find(abs(lateral_a_temp.col(1))>0);
	int Lla = x.n_rows;
	int Wla = lateral_a_temp.n_cols;
	arma::cx_mat lateral_a = arma::cx_mat(arma::zeros(Lla, Wla), arma::zeros(Lla, Wla));
	lateral_a = lateral_a_temp.submat (0, 0, size(lateral_a));
	//cout << "main_a: " << endl << main_a << endl;	
	//cout << "Lateral_a: " << endl << lateral_a << endl;
	//cout << size(main_a) << endl << size(lateral_a) << endl << size(brnches_a) << endl;
	brnches_a.cols(4,5) = join_cols(main_a,lateral_a);
	//cout << "brnches_a: " << endl << brnches_a << endl;
	//cout << Lma;

	b = Lma;
	//arma::cx_mat temp = arma::cx_mat(arma::zeros(1, 1), arma::zeros(1, 1));
	for (int i = Lma; i < Lnum_a; ++i)
	{
		//cout << i << endl;
		//cout << brnches_a(i,0) << endl;
		if(abs(brnches_a(i,0)) > b)
		{
			//cout << brnches_a(i,0) << endl;			
			for(int j = Lma; j < Lnum_a; ++j)
			{
				if(brnches_a(i,0) == brnches_a(j,1))
				{
					brnches_a(i,4) = brnches_a(j,5);
					break;
				}
			}
		}
		else
		brnches_a(i,4) = brnches_a(i,0);
	}
	//cout << "brnches_a: " << endl << brnches_a.cols(4,5) << endl;
	arma::cx_mat br_a = arma::cx_mat(arma::zeros(Lnum_a, 4), arma::zeros(Lnum_a, 4));
	//cout << size(br_a) << " " << size(brnches_a) << endl;
	br_a.col(0) = brnches_a.col(4);
	br_a.col(1) = brnches_a.col(5);
	br_a.cols(2,3) = brnches_a.cols(2,3);
	//cout << "br_a: " << endl << br_a << endl;

	arma::cx_mat main_b_temp = arma::cx_mat(arma::zeros(Lnum_b, 2), arma::zeros(Lnum_b, 2));
	main_b_temp(0,0) = brnches_b(0,0);
	main_b_temp(0,1) = brnches_b(0,1);
	//std::cout << main_b_temp << endl;
	a = 1;
	for (int i = 1; i < Lbr; ++i)
	{
		if (brnches_b(i,0) == (brnches_b(i-1,0)+cx_unity(0, 0)))
		{
			main_b_temp(a,0) = brnches_b(i,0);
			main_b_temp(a,1) = brnches_b(i,1);
			a = a+1;
		}
		else
		break;
	}
	//std::cout << main_b_temp << endl;
	int Lmb;
	int Wmb = main_b_temp.n_cols;
	x = find(abs(main_b_temp.col(1))>0);
	Lmb = x.n_rows;
	//cout << "x: " << endl << x << endl;
	//cout << Lmb << Wmb << endl;
	arma::cx_mat main_b = arma::cx_mat(arma::zeros(Lmb, Wmb), arma::zeros(Lmb, Wmb));
	
	main_b = main_b_temp.submat (0, 0, size(main_b));
	
	//cout << "brnches_b: " << endl << brnches_b << endl;
	arma::cx_mat lateral_b_temp = arma::cx_mat(arma::zeros(Lnum_b, 2), arma::zeros(Lnum_b, 2));
	a = 0;
	b = Lmb;
	for (int i = Lmb; i < Lnum_b; ++i)
	{
		if (abs(brnches_b(i,1)) > b)
		{
			lateral_b_temp(a,1) = b+1;
			a = a+1;
			b = b+1; 
		}
		else
		{
			lateral_b_temp(a,1) = brnches_b(i,1);
			a = a+1;
		}
	}
	
	x = find(abs(lateral_b_temp.col(1))>0);
	int Llb = x.n_rows;
	int Wlb = lateral_b_temp.n_cols;
	arma::cx_mat lateral_b = arma::cx_mat(arma::zeros(Llb, Wlb), arma::zeros(Llb, Wlb));
	lateral_b = lateral_b_temp.submat (0, 0, size(lateral_b));
	//cout << "main_b: " << endl << main_b << endl;	
	//cout << "Lateral_b: " << endl << lateral_b << endl;
	//cout << size(main_b) << endl << size(lateral_b) << endl << size(brnches_b) << endl;
	brnches_b.cols(4,5) = join_cols(main_b, lateral_b);
	//cout << "brnches_b: " << endl << brnches_b << endl;
	//cout << Lmb;

	b = Lmb;
	//arma::cx_mat temp = arma::cx_mat(arma::zeros(1, 1), arma::zeros(1, 1));
	for (int i = Lmb; i < Lnum_b; ++i)
	{
		//cout << i << endl;
		//cout << brnches_b(i,0) << endl;
		if(abs(brnches_b(i,0)) > b)
		{
			//cout << brnches_b(i,0) << endl;			
			for(int j = Lmb; j < Lnum_b; ++j)
			{
				if(brnches_b(i,0) == brnches_b(j,1))
				{
					brnches_b(i,4) = brnches_b(j,5);
					break;
				}
			}
		}
		else
		brnches_b(i,4) = brnches_b(i,0);
	}
	//cout << "brnches_b: " << endl << brnches_b.cols(4,5) << endl;
	arma::cx_mat br_b = arma::cx_mat(arma::zeros(Lnum_b, 4), arma::zeros(Lnum_b, 4));
	//cout << size(br_b) << " " << size(brnches_b) << endl;
	br_b.col(0) = brnches_b.col(4);
	br_b.col(1) = brnches_b.col(5);
	br_b.cols(2,3) = brnches_b.cols(2,3);
	//cout << "br_b: " << endl << br_b << endl;
	
	arma::cx_mat main_c_temp = arma::cx_mat(arma::zeros(Lnum_c, 2), arma::zeros(Lnum_c, 2));
	main_c_temp(0,0) = brnches_c(0,0);
	main_c_temp(0,1) = brnches_c(0,1);
	//std::cout << main_b_temp << endl;
	a = 1;
	for (int i = 1; i < Lbr; ++i)
	{
		if (brnches_c(i,0) == (brnches_c(i-1,0)+cx_unity(0, 0)))
		{
			main_c_temp(a,0) = brnches_c(i,0);
			main_c_temp(a,1) = brnches_c(i,1);
			a = a+1;
		}
		else
		break;
	}
	//std::cout << main_c_temp << endl;
	int Lmc;
	int Wmc = main_c_temp.n_cols;
	x = find(abs(main_c_temp.col(1))>0);
	Lmc = x.n_rows;
	//cout << "x: " << endl << x << endl;
	//cout << Lmc << Wmc << endl;
	arma::cx_mat main_c = arma::cx_mat(arma::zeros(Lmc, Wmc), arma::zeros(Lmc, Wmc));
	
	main_c = main_c_temp.submat (0, 0, size(main_c));
	
	//cout << "brnches_c: " << endl << brnches_c << endl;
	arma::cx_mat lateral_c_temp = arma::cx_mat(arma::zeros(Lnum_c, 2), arma::zeros(Lnum_c, 2));
	a = 0;
	b = Lmc;
	for (int i = Lmc; i < Lnum_c; ++i)
	{
		if (abs(brnches_c(i,1)) > b)
		{
			lateral_c_temp(a,1) = b+1;
			a = a+1;
			b = b+1; 
		}
		else
		{
			lateral_c_temp(a,1) = brnches_c(i,1);
			a = a+1;
		}
	}
	
	x = find(abs(lateral_c_temp.col(1))>0);
	int Llc = x.n_rows;
	int Wlc = lateral_c_temp.n_cols;
	arma::cx_mat lateral_c = arma::cx_mat(arma::zeros(Llc, Wlc), arma::zeros(Llc, Wlc));
	lateral_c = lateral_c_temp.submat (0, 0, size(lateral_c));
	//cout << "main_c: " << endl << main_c << endl;	
	//cout << "Lateral_c: " << endl << lateral_c << endl;
	//cout << size(main_c) << endl << size(lateral_c) << endl << size(brnches_c) << endl;
	brnches_c.cols(4,5) = join_cols(main_c, lateral_c);
	//cout << "brnches_c: " << endl << brnches_c << endl;
	//cout << Lmc;

	b = Lmc;
	//arma::cx_mat temp = arma::cx_mat(arma::zeros(1, 1), arma::zeros(1, 1));
	for (int i = Lmc; i < Lnum_c; ++i)
	{
		//cout << i << endl;
		//cout << brnches_c(i,0) << endl;
		if(abs(brnches_c(i,0)) > b)
		{
			//cout << brnches_c(i,0) << endl;			
			for(int j = Lmc; j < Lnum_c; ++j)
			{
				if(brnches_c(i,0) == brnches_c(j,1))
				{
					brnches_c(i,4) = brnches_c(j,5);
					break;
				}
			}
		}
		else
		brnches_c(i,4) = brnches_c(i,0);
	}
	//cout << "brnches_c: " << endl << brnches_c.cols(4,5) << endl;
	arma::cx_mat br_c = arma::cx_mat(arma::zeros(Lnum_c, 4), arma::zeros(Lnum_c, 4));
	//cout << size(br_c) << " " << size(brnches_c) << endl;
	br_c.col(0) = brnches_c.col(4);
	br_c.col(1) = brnches_c.col(5);
	br_c.cols(2,3) = brnches_c.cols(2,3);
	//cout << "br_c: " << endl << br_c << endl;
	//**************end of branch renaming**************//

	//calculate yy modified
	arma::cx_mat YY_a = arma::cx_mat(arma::zeros(1, Lnum_a), arma::zeros(1, Lnum_a));
	arma::cx_mat YY_b = arma::cx_mat(arma::zeros(1, Lnum_b), arma::zeros(1, Lnum_b));
	arma::cx_mat YY_c = arma::cx_mat(arma::zeros(1, Lnum_c), arma::zeros(1, Lnum_c));
	arma::cx_mat alpha_a = arma::cx_mat(arma::zeros(1, Lnum_a), arma::zeros(1, Lnum_a));
	arma::cx_mat alpha_b = arma::cx_mat(arma::zeros(1, Lnum_b), arma::zeros(1, Lnum_b));
	arma::cx_mat alpha_c = arma::cx_mat(arma::zeros(1, Lnum_c), arma::zeros(1, Lnum_c));

	//arma::cx_mat cx_unity = arma::cx_mat(arma::ones(1, 1), arma::zeros(1, 1));

	for (int i = 0; i < Lnum_a; ++i)
	{
		YY_a(0, i) = cx_unity(0, 0) / brnches_a(i, 2);
		alpha_a(0, i) = brnches_a(i, 3);

	}
	//cout << YY_a<< endl;
	for (int i = 0; i < Lnum_b; ++i)
	{
		YY_b(0, i) = cx_unity(0, 0) / brnches_b(i, 2);
		alpha_b(0, i) = brnches_b(i, 3);

	}
	//cout << YY_b<< endl;
	for (int i = 0; i < Lnum_c; ++i)
	{
		YY_c(0, i) = cx_unity(0, 0) / brnches_c(i, 2);
		alpha_c(0, i) = brnches_c(i, 3);

	}
	//cout << YY_c<< endl;

	//element in Y of three phase are different, then treat separately 
	//phase A

	arma::cx_mat AA;
	AA << brnches_a(0, 0) << arma::endr;
	arma::cx_mat BB = brnches_a.col(1);
	arma::cx_mat ka = join_cols(AA, BB);
	//cout << ka << endl;
	arma::cx_mat Y_a = arma::cx_mat(arma::zeros(Lnum_a + 1, Lnum_a + 1), arma::zeros(Lnum_a + 1, Lnum_a + 1));
	//int n = 0;
	for (int m = 0; m < (Lnum_a + 1); ++m)
	{
		for (int n = 0; n < (Lnum_a + 1); ++n)
		{
			//cout << "m=" << m << "n=" << n << endl;
			if (m == n)
			{
				for (int x = 0; x < Lnum_a; ++x)
				{
					//if ((int)(real(brnches_a(x, 0))) == (int)real(ka(m)) || (int)(real(brnches_a(x, 1))) == (int)real(ka(m)))
						//Y_a(m, n) = Y_a(m, n) + YY_a(x);

					//Modified by Valli on 12/24/2017
					if ((int)(real(brnches_a(x, 1))) == (int)real(ka(m)))
						Y_a(m, n) = Y_a(m, n) + YY_a(x)/(alpha_a(x)*alpha_a(x));
					
					if ((int)(real(brnches_a(x, 0))) == (int)real(ka(m)))
						Y_a(m, n) = Y_a(m, n) + YY_a(x);
				}
			}
			else
			{
				for (int x = 0; x < Lnum_a; ++x)
				{
					if ((int)(real(brnches_a(x, 0))) == (int)real(ka(m)) && (int)(real(brnches_a(x, 1))) == (int)real(ka(n)))
						Y_a(m, n) = Y_a(m, n) - YY_a(x)/alpha_a(x); //modified by Valli 12/24/2017
				}
				for (int x = 0; x < Lnum_a; ++x)
				{
					if ((int)(real(brnches_a(x, 1))) == (int)real(ka(m)) && (int)(real(brnches_a(x, 0))) == (int)real(ka(n)))
						Y_a(m, n) = Y_a(m, n) - YY_a(x)/alpha_a(x); //modified by Valli 12/24/2017
				}
			}
		}
	}
	//cout <<real(Y_a.col(1))<< endl;

	//phase B
	arma::cx_mat AB;
	AB << brnches_b(0, 0) << arma::endr;
	arma::cx_mat BBB = brnches_b.col(1);
	arma::cx_mat kb = join_cols(AB, BBB);
	arma::cx_mat Y_b = arma::cx_mat(arma::zeros(Lnum_b + 1, Lnum_b + 1), arma::zeros(Lnum_b + 1, Lnum_b + 1));
	//int n = 0;
	for (int m = 0; m < (Lnum_b + 1); ++m)
	{
		for (int n = 0; n < (Lnum_b + 1); ++n)
		{
			//cout << "m=" << m << "n=" << n << endl;
			if (m == n)
			{
				for (int x = 0; x < Lnum_b; ++x)
				{
					//if ((int)(real(brnches_b(x, 0))) == (int)real(kb(m)) || (int)(real(brnches_b(x, 1))) == (int)real(kb(m)))
						//Y_b(m, n) = Y_b(m, n) + YY_b(x);

					//Modified by Valli on 12/24/2017
					if ((int)(real(brnches_b(x, 1))) == (int)real(kb(m)))
						Y_b(m, n) = Y_b(m, n) + YY_b(x)/(alpha_b(x)*alpha_b(x));
					
					if ((int)(real(brnches_b(x, 0))) == (int)real(kb(m)))
						Y_b(m, n) = Y_b(m, n) + YY_b(x);
				}
			}
			else
			{
				for (int x = 0; x < Lnum_b; ++x)
				{
					if ((int)(real(brnches_b(x, 0))) == (int)real(kb(m)) && (int)(real(brnches_b(x, 1))) == (int)real(kb(n)))
						//Y_b(m, n) = Y_b(m, n) - YY_b(x);
						Y_b(m, n) = Y_b(m, n) - YY_b(x)/alpha_b(x); //Modified by Valli on 12/24/2017
				}
				for (int x = 0; x < Lnum_b; ++x)
				{
					if ((int)(real(brnches_b(x, 1))) == (int)real(kb(m)) && (int)(real(brnches_b(x, 0))) == (int)real(kb(n)))
						//Y_b(m, n) = Y_b(m, n) - YY_b(x);
						Y_b(m, n) = Y_b(m, n) - YY_b(x)/alpha_b(x); //Modified by Valli on 12/24/2017
				}
			}
		}
	}
	//cout <<real(Y_b.col(3))<< endl;

	//phase C
	arma::cx_mat AC;
	AC << brnches_c(0, 0) << arma::endr;
	arma::cx_mat BC = brnches_c.col(1);
	arma::cx_mat kc = join_cols(AC, BC);
	arma::cx_mat Y_c = arma::cx_mat(arma::zeros(Lnum_c + 1, Lnum_c + 1), arma::zeros(Lnum_c + 1, Lnum_c + 1));
	//int n = 0;
	for (int m = 0; m < (Lnum_c + 1); ++m)
	{
		for (int n = 0; n < (Lnum_c + 1); ++n)
		{
			//cout << "m=" << m << "n=" << n << endl;
			if (m == n)
			{
				for (int x = 0; x < Lnum_c; ++x)
				{
					//if ((int)(real(brnches_c(x, 0))) == (int)real(kc(m)) || (int)(real(brnches_c(x, 1))) == (int)real(kc(m)))
						//Y_c(m, n) = Y_c(m, n) + YY_c(x);
				
					//Modified by Valli on 12/24/2017
					if ((int)(real(brnches_c(x, 1))) == (int)real(kc(m)))
						Y_c(m, n) = Y_c(m, n) + YY_c(x)/(alpha_c(x)*alpha_c(x));
					
					if ((int)(real(brnches_c(x, 0))) == (int)real(kc(m)))
						Y_c(m, n) = Y_c(m, n) + YY_c(x);
				}
			}
			else
			{
				for (int x = 0; x < Lnum_c; ++x)
				{
					if ((int)(real(brnches_c(x, 0))) == (int)real(kc(m)) && (int)(real(brnches_c(x, 1))) == (int)real(kc(n)))
						//Y_c(m, n) = Y_c(m, n) - YY_c(x);
						Y_c(m, n) = Y_c(m, n) - YY_c(x)/alpha_c(x); //Modified by Valli on 12/24/2017
				}
				for (int x = 0; x < Lnum_c; ++x)
				{
					if ((int)(real(brnches_c(x, 1))) == (int)real(kc(m)) && (int)(real(brnches_c(x, 0))) == (int)real(kc(n)))
						//Y_c(m, n) = Y_c(m, n) - YY_c(x);
						Y_c(m, n) = Y_c(m, n) - YY_c(x)/alpha_c(x); //Modified by Valli on 12/24/2017
				}
			}
		}
	}
	//cout <<real(Y_c.col(13))<< endl;

	arma::cx_mat Nnum1 = arma::cx_mat(arma::zeros(1, 1), arma::zeros(1, 1));
	Nnum1 = max(join_cols(brnches.col(1), brnches.col(0)), 0);

	int Nnum = (int)real(Nnum1(0, 0)) + 1;// NodeNo starts form 0, count of nodes should +1
	//cout << Nnum << endl;
	arma::mat Lnum = arma::zeros(1, 3);
	Lnum << Lnum_a << Lnum_b << Lnum_c << arma::endr;

	y_results.Lnum = Lnum;
	y_results.Lnum_a = Lnum_a;
	y_results.Lnum_b = Lnum_b;
	y_results.Lnum_c = Lnum_c;
	y_results.Nnum = Nnum;
	y_results.Y_a = Y_a;
	y_results.Y_b = Y_b;
	y_results.Y_c = Y_c;

	//mat rY_a = real(Y_a);
	//rY_a.save("Ya.mat", raw_ascii);
	//mat rY_b = real(Y_b);
	//rY_b.save("Yb.mat", raw_ascii);
	//mat rY_c = real(Y_c);
	//rY_c.save("Yc.mat", raw_ascii);
	return y_results;
	
}
