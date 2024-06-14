/* soft limiter using a scaled arctangent
 * evie/ULRIKE, 2024
 */

#include "m_pd.h"
#include "stdlib.h"
#include "math.h"

const float TWO_OVER_PI = 2.0 / M_PI;

static t_class * softlimtan_tilde_class;

typedef struct _softlimtan_tilde {
	t_object x_obj;
	t_float f;		// backup float for storing float type input
					// in case the audio inlet receives float type
					// input by mistake

	//creating 'extra' inlets and outlets beyond the expected single audio inlet
	t_inlet *filter_threshold;
	t_outlet *signal_out;
} t_softlimtan_tilde;

//the workhorse dsp method where operations on the incoming vectors actually happen
t_int *softlimtan_tilde_perform(t_int *w) {
	//for some reason, w[] is indexed from 1, not 0.

	//fetching things as they were defined in the dsp_add function call
	//the class dataspace
	t_softlimtan_tilde *x 		=	(t_softlimtan_tilde *)	(w[1]);
	//the next two signal vectors, in and out
	t_sample *signalin			=	(t_sample *)			(w[2]);
	t_sample *signalout			=	(t_sample *)			(w[3]);
	t_sample *filterthreshold	=	(t_sample *)			(w[4]);
	//the vector size
	int n						=	(int)					(w[5]);

	//looping through the whole vector to do an operation on each sample
	for(int i = 0; i < n; i++) {
		//bounding filter threshold between [0,1]
		if(filterthreshold[i] < 0.0) {
			filterthreshold[i] = 0.0;
		}
		else if(filterthreshold[i] > 1.0) {
			filterthreshold[i] = 1.0;
		}
		if(signalin[i] < -(filterthreshold[i])) {
			//arctan shifted and scaled to give a delta of 1 at the threshold point and
			//  asymptotes at -1 and 1
			float one_minus_threshold = 1.0 - filterthreshold[i];
			signalout[i] = -(one_minus_threshold *
				(TWO_OVER_PI * atan(((-(signalin[i]) - filterthreshold[i]) / (one_minus_threshold)) * M_PI_2)) +
				filterthreshold[i]);
		}
		//between thresholds the signal comes through untouched
		else if(signalin[i] >= -(filterthreshold[i]) && signalin[i] <= filterthreshold[i]) {
			signalout[i] = signalin[i];
		}
		else if(signalin[i] > filterthreshold[i]) {
			//arctan shifted and scaled to give a delta of 1 at the threshold point and
			//  asymptotes at -1 and 1
			float one_minus_threshold = 1.0 - filterthreshold[i];
			signalout[i] = one_minus_threshold *
				(TWO_OVER_PI * atan(((signalin[i] - filterthreshold[i]) / (one_minus_threshold)) * M_PI_2)) +
				filterthreshold[i];
		}
	}

	//returning the pointer we were passed,
	//+ number of arguments we defined + 1
	return (w+6);
}

//handler for dsp input on the leftmost inlet (the default architecture when
//dealing with a single dsp inlet)
void softlimtan_tilde_dsp(t_softlimtan_tilde *x, t_signal **sp) {
	//adding the 'perform-routine' to the DSP-tree.
	//after the perform function, number of following arguments, and the
	//class dataspace are passed, the order of vector pointers in the signal
	//array is determined here as follows:
		//1: incoming vector, 2: outgoing vector, and 3: vector size
	dsp_add(softlimtan_tilde_perform, 5, x,
		sp[0]->s_vec, sp[1]->s_vec, sp[2]->s_vec, sp[0]->s_n);
}

// handler for 'bang' type inputs
void softlimtan_tilde_bang(t_softlimtan_tilde *x) {
}

//destructor called when an instance is destroyed
void softlimtan_tilde_free(t_softlimtan_tilde * x) {
	inlet_free(x->filter_threshold);
	outlet_free(x->signal_out);
}

//constructor called when a new instance is made
void *softlimtan_tilde_new() {
	t_softlimtan_tilde *x = (t_softlimtan_tilde *) pd_new(softlimtan_tilde_class);

	//creating an audio inlet
	x->filter_threshold = inlet_new(&x->x_obj, &x->x_obj.ob_pd, &s_signal, &s_signal);

	//creating an audio outlet
	x->signal_out = outlet_new(&x->x_obj, &s_signal);

	return (void *) x;
}

//called when pure data is loading libraries during startup
void softlimtan_tilde_setup(void) {
	softlimtan_tilde_class = class_new(gensym("softlimtan~"),
		(t_newmethod)softlimtan_tilde_new,
        (t_method)softlimtan_tilde_free,
        sizeof(t_softlimtan_tilde),
        CLASS_DEFAULT, A_GIMME, 0);

	//binding the dsp handler function to the mandatory symbol "dsp"
	class_addmethod(softlimtan_tilde_class,
		(t_method)softlimtan_tilde_dsp,
		gensym("dsp"),
		A_CANT, 0);
	//do class stuff so we can have a signal inlet, I think
	CLASS_MAINSIGNALIN(softlimtan_tilde_class,
		t_softlimtan_tilde, f);
	//binding other symbols to their handler functions
	class_addbang(softlimtan_tilde_class,
		(t_method) softlimtan_tilde_bang);
}