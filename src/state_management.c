#include <stdint.h>
#include "state_management.h"

PhaseState next_phase_state(PhaseState currentState, uint32_t election_clock) {
    switch (currentState) {
        case DATA_COLLECTION:
            if ((election_clock % ELECTION_RATE_MS) == 0) {
                return CLUSTER_HEAD_SELECTION;
            }
            else return DATA_COLLECTION;
            break;
        case CLUSTER_HEAD_SELECTION:
            return DETERMINE_CLUSTER_HEAD;
            break;
        case DETERMINE_CLUSTER_HEAD:
            if ((election_clock % ELECTION_LENGTH_MS) == 0) {
                return AWAKE_PAIR_SELECTION;
            }
            else return DETERMINE_CLUSTER_HEAD;
            break;
        case AWAKE_PAIR_SELECTION:
            return DATA_COLLECTION;
            break;
        default:
            return DATA_COLLECTION;
            break;
    }
    
}