// Copyright Strati D. Zerbinis, 2025. All Rights Reserved.

using System;

public class Class1
{
	public Class1()
	{ }

	public float AdjustTau0()
	{
		float a, b, c, d, p0, p1, p2, p3, p4, tau_0, tau_1, beta_0, beta_1;
		float delta_m1, delta_0, delta_1;
		float t, t_sq, t_qb, delta_m1_sq, delta_0_sq, delta_1_sq;
		float term0, term1, term2, term3, term4, term5, term6, term7, term8, term9, term10, term11, term12, term13, term14, term15, term16, term17, term18, term19, term20, term21;
		float numerator, denominator;


		a = 0.5f * (1.0f - tau_0) * (1.0f + beta_0);
		b = 0.5f * (1.0f - tau_0) * (1.0f - beta_0);
		c = 0.5f * (1.0f - tau_1) * (1.0f + beta_1);
		d = 0.5f * (1.0f - tau_1) * (1.0f - beta_1);

		delta_m1 = p1 - p0;
		delta_0 = p2 - p1;
		delta_1 = p3 - p2;

        t_sq = t * t;
        t_qb = t_sq * t;


        delta_m1_sq = delta_m1 * delta_m1



		delta_0_sq = delta_0 * delta_0;
		delta_1_sq = delta_1 * delta_1;

		term0 = t - 1.0f;
		term1 = term0 * term0;
		term2 = delta_0 + delta_1;

		term3 = delta_m1 * term1 * t * (beta_0 + 1.0f) * delta_0_sq * delta_1 * term2;

		term4 = 2.0f * delta_m1 * delta_0;
		term5 = d * -delta_m1 * term0 * t_sq * delta_0_sq;
		term6 = delta_0 * delta_1;
		term7 = p4 + p2 * t_sq * ((2.0f * t) - 3.0f) - p1 * term1 * ((2.0f * t) + 1.0f);
		term8 = p2 * t_sq * (((2.0f * t) - 3.0f) - (c * t) + c);
		term9 = p1 * term0 * (t + 1.0f + (c - 2.0f) * t_sq);
		term10 = term4 * (term5 + (term7 * term6) + ((p4 + term8 + term9) * delta_m1_sq));

		term11 = (-2.0f * d) * -delta_1 * term0 * t_sq * delta_0_sq;
		term12 = p1 * term1 * ((3.0f * t) + 2.0f);
		term13 = p2 * t * (1.0f + (4.0f - (3.0f * t)) * t);
		term14 = -delta_0 * term1 * t * beta_0;
		term15 = ((-2.0f * p4) + term12 + term13 + term14) * term6;
		term16 = 1.0f + ((4.0f + ((2.0f * c) * term0) - (3.0f * t)) * t);
		term17 = 2.0f + t + (((2.0f * c) - 3.0f) * t_sq);
		term18 = (-2.0f * p4) + (p2 * t * term16) - (p1 * term0 * term17) + (-delta_0 * term1 * t * beta_0);
		term19 = delta_m1_sq * (term11 + term15 + (term18 * delta_1_sq));

		term20 = (-delta_0 * (beta_0 - 1) * delta_m1_sq);
		term21 = (delta_m1 * (beta_0 + 1) * delta_0_sq);


        numerator = term3 - term10 + term19;
		denominator = term1 * t * (term20 + term21) * delta_1 * term2;

        tau_0 = numerator / denominator;

		return tau_0;
    }

	public void AdjustTau1()
	{
		float a, b, c, d, p0, p1, p2, p3, p4, tau_0, tau_1, beta_0, beta_1;
		float delta_m1, delta_0, delta_1;
		float t, t_sq, t_qb, delta_m1_sq, delta_0_sq, delta_1_sq;
		float term0, term1, term2, term3, term4, term5, term6, term7, term8, term9, term10, term11, term12, term13, term14, term15, term16, term17, term18, term19, term20, term21;
		float numerator, denominator;


		a = 0.5f * (1.0f - tau_0) * (1.0f + beta_0);
		b = 0.5f * (1.0f - tau_0) * (1.0f - beta_0);
		c = 0.5f * (1.0f - tau_1) * (1.0f + beta_1);
		d = 0.5f * (1.0f - tau_1) * (1.0f - beta_1);

		delta_m1 = p1 - p0;
		delta_0 = p2 - p1;
		delta_1 = p3 - p2;

		t_sq = t * t;
		t_qb = t_sq * t;


		delta_m1_sq = delta_m1 * delta_m1





		delta_0_sq = delta_0 * delta_0;
		delta_1_sq = delta_1 * delta_1;

		term0 = t - 1.0f;
		term1 = term0 * term0;




		numerator = -1 * (
			2 * a * -delta_m1 * term1 * t * delta_0_sq * delta_1 * (delta_0 + delta_1) +
			delta_m1 * delta_0 * (
				delta_1 * term0 * t_sq * (beta_1 - 1) * delta_0_sq +
				2 * (
					p4 + p2 * t_sq * ((2 * t) - 3) -
					p1 * term1 * ((2 * t) - 1)
				) * delta_0 * delta_1 +
				(
					-2 * p1 +
					2 * p4 +
					5 * -delta_0 * t_sq +
					3 * delta_0 * t_qb +
					-delta_0 * term0 * t_sq * beta_1
				) * delta_1_sq
			) + 
			delta_m1_sq * (
                delta_1 * term0 * t_sq * (beta_1 - 1) * delta_0_sq +
				2 * (
					p4 +
					p1 * term1 * (((b - 2) * t) -1) -
					p2 * t * (
						b * term1 +
						(3 - (2 * t)) * t
					)
				) * delta_0 * delta_1 +
				(
					2 * p4 +
					p1 * term0 * (
						2 +
						(
							2 +
							2 * b * term0 -
							3 * t
						) * t
					) +
					p2 * t * ( 
						-2 * b *term1 +
						t * ((3 * t) -5)
					) +
					-delta_0 * term0 * t_sq * beta_1 
				) * delta_1_sq
			)
        );

		denominator =
			term0 * t_sq * delta_m1 * (delta_m1 + delta_0) * (
				-delta_1 * (beta_1 - 1) * delta_0_sq +
				delta_0 * (beta_1 + 1) * delta_1_sq
			);

        tau_1 = numerator / denominator;

        return tau_1;
    }



    public void SamplePoint()
	{
		float a, b, c, d, p0, p1, p2, p3, p4, tau_0, tau_1, beta_0, beta_1;
		float delta_m1, delta_0, delta_1;
		float t, t_sq, t_qb, delta_m1_sq, delta_0_sq, delta_1_sq;
		float term0, term1, term2, term3, term4, term5, term6, term7, term8, term9, term10, term11, term12, term13, term14, term15, term16,term17,term18,term19,term20;
		float numerator, denominator;



        a = 0.5f * (1.0f - tau_0) * (1.0f + beta_0);
        b = 0.5f * (1.0f - tau_0) * (1.0f - beta_0);
        c = 0.5f * (1.0f - tau_1) * (1.0f + beta_1);
        d = 0.5f * (1.0f - tau_1) * (1.0f - beta_1);

        delta_m1 = p1 - p0;
		delta_0 = p2 - p1;
		delta_1 = p3 - p2;

		t_sq = t * t;
		t_qb = t_sq * t;

        delta_m1_sq = delta_m1 * delta_m1

		delta_0_sq = delta_0 * delta_0;
		delta_1_sq = delta_1 * delta_1;


		term0 = t - 1.0f;
		term1 = term0 * term0;

		term2 = (delta_0 + delta_1);
		term3 = (delta_m1 + delta_0);

		term4 = (-a * (-delta_m1) * term1 * t * delta_m1_sq * delta_1 * term2) ;

		term5 = (-d * (-delta_1) * term0 * t_sq * delta_0_sq);
		term6 = (3.0f - 2.0f * t);
		term7 = (((p2 * term6 * t_sq) + (p1 * term1 * (1.0f + (2.0f * t)))) * delta_0 * delta_1);
		term8 = (((p1 + (c - 3.0f) * (-delta_0) * t_sq) + ((c - 2.0f) * delta_0 * t_qb)) * delta_1_sq);
		term9 = (delta_m1 * delta_0 * (term5 + term7 + term8));

		term10 = (-d * (-delta_1) * term0 * t_sq * delta_0_sq);
		term11 = (-p1 * term1 * (((b - 2.0f) * t) - 1.0f));
		term12 = (p2 * t * ((b * term1) + (term6 * t)));
		term13 = ((term11 + term12) * delta_0 * delta_1);
		term14 = (b * delta_0 * t);
		term15 = ((c + (2.0f * b) - 3.0f) * (-delta_0) * t_sq);
		term16 = ((c + b - 2.0f) * delta_0 * t_qb);
		term17 = ((p1 + term14 + term15 + term16) * delta_1_sq);

		term18 = (delta_m1_sq * ( term10 + term13 + term17 ));


		numerator = term4 + term9 + term18;
		denominator = delta_m1 * term3 * delta_1 * term2;

		p4 = numerator / denominator;

        //*******************************************************************************************************

   //     float _numerator =
			//(-a * (p0 - p1) * term1 * t * delta_m1_sq * delta_1 * (delta_0 + delta_1))
			//+
			//(delta_m1 * delta_0 *
			//	(
			//		(-d * (p2 - p3) * term0 * t_sq * delta_0_sq)
			//		+
			//		(((p2 * (3.0f - 2.0f * t) * t_sq) + (p1 * term1 * (1.0f + 2.0f * t))) * delta_0 * delta_1)
			//		+
			//		(((p1 + (c - 3.0f) * (p1 - p2) * t_sq) + ((c - 2.0f) * delta_0 * t_qb)) * delta_1_sq)
			//	)
			//)
			//+
			//(delta_m1_sq *
			//	(
			//		(-d * (p2 - p3) * term0 * t_sq * delta_0_sq)
			//		+
			//		((
			//			(-p1 * term1 * (((b - 2.0f) * t) - 1.0f))
			//			+
			//			(p2 * t * ( (b * term1)
			//					+
			//					((3.0f - 2.0f * t) * t) ))
			//		) * delta_0 * delta_1)
			//		+
			//		((
			//			p1
			//			+
			//			(b * delta_0 * t)
			//			+
			//			((c + (2.0f * b) - 3.0f) * (p1 - p2) * t_sq)
			//			+
			//			((c + b - 2.0f) * delta_0 * t_qb)
			//		) * delta_1_sq)
			//	))
			//	;
    }
}
