#include <carlsim.h>		// include CARLsim
#include <limits.h>
#include "gtest/gtest.h"	// include Google testing scripts

// Don't forget to set REGRESSION_TESTING flag to 1 in config.h 

// TODO: figure out test directory organization (see issue #67); group into appropriate test cases; have test cases
// for published results; add documentation; etc.

// TODO: test interface (see issue #38)

// TODO: add speed test scripts (see issue #32)

// TODO: add more tests in general (see issue #21)

/*
 * GENERAL TESTING STRATEGY
 * ------------------------
 *
 * We provide test cases to A) test core functionality of CARLsim, to B) test the reproducibility of published results,
 * and C) to benchmark simulation speed.
 *
 * A) TESTING CORE FUNCTIONALITY
 * 1. Test core data structures when some functionality is enabled.
 *    For example: Set STP to true for a specific group, check grp_Info to make sure all values are set accordingly.
 * 2. Test core data structures when some functionality is disabled.
 *    For example: Set STP to false for a specific group, check grp_Info to make sure it's disabled.
 * 3. Test behavior when values for input arguments are chosen unreasonably.
 *    For example: Create a group with N=-4 (number of neurons) and expect simulation to die. This is because each
 *    core function should have assertion statements to prevent the simulation from running unreasonable input values.
 *    In some cases, it makes sense to catch this kind of error in the user interface as well (and display an
 *    appropriate error message to the user), but these tests should be placed in the UserInterface test case.
 * 4. Test behavior of network when run with reasonable values.
 *    For example: Run a sample network with STP enabled, and check stpu[nid] and stpx[nid] to make sure they behave.
 *    as expected. You can use the PeriodicSpikeGenerator to be certain of specific spike times and thus run
 *    reproducible sample networks.
 * 5. Test behavior of network when run in CPU mode vs. GPU mode.
 *    For example: Run a sample network with STP enabled, once in CPU mode and once in GPU mode. Record stpu[nid] and
 *    stpx[nid], and make sure that both simulation mode give the exact same result (except for some small error
 *    margin that can account for rounding errors/etc.).
 *
 * B) TESTING PUBLISHED RESULTS
 *
 * C) BENCHMARK TESTS
 *
 */


/// **************************************************************************************************************** ///
/// COMMON
/// **************************************************************************************************************** ///

//! a periodic spike generator (constant ISI) creating spikes at a certain rate
class PeriodicSpikeGenerator : public SpikeGenerator {
public:
	PeriodicSpikeGenerator(float rate) {
		assert(rate>0);
		rate_ = rate;	  // spike rate
		isi_ = 1000/rate; // inter-spike interval in ms
	}

	unsigned int nextSpikeTime(CpuSNN* snn, int grpId, int nid, unsigned int currentTime) {
		return currentTime+isi_; // periodic spiking according to ISI
	}

private:
	float rate_;	// spike rate
	int isi_;		// inter-spike interval that results in above spike rate
};


//! a spike monitor that counts the number of spikes per neuron, and also the total number of spikes
//! used to test the behavior of SpikeCounter
class SpikeMonitorPerNeuron: public SpikeMonitor {
private:
	const int nNeur_; // number of neurons in the group
	int* spkPerNeur_; // number of spikes per neuron
	long long spkTotal_; // number of spikes in group (across all neurons)

public:
	SpikeMonitorPerNeuron(int numNeur) : nNeur_(numNeur) {
		// we're gonna count the spikes each neuron emits
		spkPerNeur_ = new int[nNeur_];
		memset(spkPerNeur_,0,sizeof(int)*nNeur_);
		spkTotal_ = 0;
	}
		
	// destructor, delete all dynamically allocated data structures
	~SpikeMonitorPerNeuron() { delete spkPerNeur_; }
		
	int* getSpikes() { return spkPerNeur_; }
	long long getSpikesTotal() { return spkTotal_; }

	// the update function counts the spikes per neuron in the current second
	void update(CpuSNN* s, int grpId, unsigned int* NeuronIds, unsigned int *timeCounts) {
		int pos = 0;
		for (int t=0; t<1000; t++) {
			for (int i=0; i<timeCounts[t]; i++,pos++) {
				// turns out id will be enumerated between 0..numNeur_; it is NOT preSynIds[]
				// or postSynIds[] or whatever...
				int id = NeuronIds[pos];
				assert(id>=0); assert(id<nNeur_);
				spkPerNeur_[id]++;
				spkTotal_++;
			}
		}
	}
};

