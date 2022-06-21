/* slew filter, produces a moving average of the samples passed through
 * evie/ULRIKE, 2022
 */

#include "m_pd.h"

static t_class * slewfilter_tilde_class;

typedef struct _slewfilter_tilde {
	t_object x_obj;
	t_float f;		// backup float for storing float type input
					// in case the audio inlet receives float type
					// input by mistake

	//internal variables
	float currentaverage;

	//externally set variables
	float mixratio;

	//creating 'extra' inlets and outlets beyond the expected single audio inlet
	t_inlet *mixratio_in;
	t_outlet *signal_out;
} t_slewfilter_tilde;

//the workhorse dsp method where operations on the incoming vectors actually happen
t_int *slewfilter_tilde_perform(t_int *w) {
	//for some reason, w[] is indexed from 1, not 0.

	//fetching things as they were defined in the dsp_add function call
	//the class dataspace
	t_slewfilter_tilde *x 		=	(t_slewfilter_tilde *)	(w[1]);
	//the next two signal vectors, in and out
	t_sample *signalin			=	(t_sample *)			(w[2]);
	t_sample *signalout			=	(t_sample *)			(w[3]);
	//the vector size
	int n						=	(int)					(w[4]);

	//looping through the whole vector to do an operation on each sample
	for(int i = 0; i < n; i++) {
		x->currentaverage = (signalin[i] * (1.0 - x->mixratio)) +
			(x->currentaverage * x->mixratio);
		signalout[i] = x->currentaverage;
	}

	//returning the pointer we were passed,
	//+ number of arguments we defined + 1
	return (w+5);
}

//handler for dsp input on the leftmost inlet (the default architecture when
//dealing with a single dsp inlet)
void slewfilter_tilde_dsp(t_slewfilter_tilde *x, t_signal **sp) {
	//adding the 'perform-routine' to the DSP-tree.
	//after the perform function, number of following arguments, and the
	//class dataspace are passed, the order of vector pointers in the signal
	//array is determined here as follows:
		//1: incoming vector, 2: outgoing vector, and 3: vector size
	dsp_add(slewfilter_tilde_perform, 4, x,
		sp[0]->s_vec, sp[1]->s_vec, sp[0]->s_n);
}

//handler for inputs beginning with the symbol "mixratio", constrains input
//to float between 0.0 and 1.0
void slewfilter_tilde_mixratio(t_slewfilter_tilde *x, t_floatarg f) {
	x->mixratio = f < 0.0 ? 0.0 : (f > 1.0 ? 1.0 : f);
}

//destructor called when an instance is destroyed
void slewfilter_tilde_free(t_slewfilter_tilde * x) {
	outlet_free(x->signal_out);
	inlet_free(x->mixratio_in);
}

//constructor called when a new instance is made
void *slewfilter_tilde_new() {
	t_slewfilter_tilde *x = (t_slewfilter_tilde *) pd_new(slewfilter_tilde_class);

	//creating a float inlet
	x->mixratio_in = inlet_new(&x->x_obj, &x->x_obj.ob_pd,
		gensym("float"), gensym("mixratio"));

	//creating an audio outlet
	x->signal_out = outlet_new(&x->x_obj, &s_signal);

	//initializing internal variables
	x->currentaverage = 0.0;
	x->mixratio = 0.0;

	return (void *) x;
}

//called when pure data is loading libraries during startup
void slewfilter_tilde_setup(void) {
	slewfilter_tilde_class = class_new(gensym("slewfilter~"),
		(t_newmethod)slewfilter_tilde_new,
        (t_method)slewfilter_tilde_free,
        sizeof(t_slewfilter_tilde),
        CLASS_DEFAULT, A_GIMME, 0);

	//binding the dsp handler function to the mandatory symbol "dsp"
	class_addmethod(slewfilter_tilde_class,
		(t_method)slewfilter_tilde_dsp,
		gensym("dsp"),
		A_CANT, 0);
	CLASS_MAINSIGNALIN(slewfilter_tilde_class,
		t_slewfilter_tilde, f);
	//binding symbol 'mixratio' to its handler function, expecting float input
	class_addmethod(slewfilter_tilde_class,
        (t_method)slewfilter_tilde_mixratio, gensym("mixratio"),
        A_DEFFLOAT, 0);
}