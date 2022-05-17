
struct context_common_s {
};

struct context_common_s context_common;

// ATTN context
struct context_attn_s {
  int  ready;
  context_attn_s() {
     ready = 0;
  }
};

struct context_attn_s context_attn;

struct context_bully_s {
  long long elections_start_time = 0;
  long long coordinator_check_start_time = 0;
  long long max_waiting_time = 100;
  bool start_elections = 0;
  bool winner = 0;
  bool waiting_for_alive = 0;
  bool waiting_for_coordinator = 0;
  int coordinator = -1;
} context_bully;

