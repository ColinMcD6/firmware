#ifndef _STATE_MANAGEMENT_H
#define	_STATE_MANAGEMENT_H

#define ELECTION_RATE_MS 25000UL //have election every 25 seconds
#define ELECTION_LENGTH_MS 2000UL //elections last 2 seconds

typedef enum {
    DATA_COLLECTION,
    CLUSTER_HEAD_SELECTION,
    DETERMINE_CLUSTER_HEAD,
    AWAKE_PAIR_SELECTION,
} PhaseState;

PhaseState next_phase_state(PhaseState currentState, uint32_t election_clock);

#endif // _STATE_MANAGEMENT_H