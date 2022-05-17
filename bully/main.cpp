#include "DSSimul.h"
//#include <unistd.h>
#include <assert.h>
#include <sstream>

int workFunction_TEST(Process *dp, Message m) {
    string s = m.getString();
    NetworkLayer *nl = dp->networkLayer;
    if (!dp->isMyMessage("TEST", s)) return false;
    set<int> neibs = dp->neibs(); 
    if (s == "TEST_HELLO") {
        int val = m.getInt();
        printf("TEST[%d]: HELLO %d message received from %d\n", dp->node, val, m.from);
        // Рассылаем сообщение соседям
        if (val < 2) {
            for (auto n: neibs) {
                nl->send(dp->node, n, Message("TEST_HELLO", val+1));
            }
        } else {
            for (auto n: neibs) {
                nl->send(dp->node, n, Message("TEST_BYE"));
            }
        }
    } else if (s == "TEST_BYE") {
        printf("TEST[%d]: BYE message received from %d\n", dp->node, m.from);
    }
    return true;
}


int init_elections(Process *dp) {
    NetworkLayer * nl = dp->networkLayer;
    set<int> neibs = dp->neibs();
    int cnt = 0;
    for (int neib : neibs) {
        if (neib > dp->node) {
            cnt++;
        }
    }

    if (cnt) {
        for (int neib : neibs) {
            if (neib > dp->node) {
                int res = nl->send(dp->node, neib, Message("Elections_init"));
                printf("[%d]: Sent Elections init to [%d] with code %d\n", dp->node, neib, res);
            }
        }

        dp->context_bully.waiting_for_alive = true;
        dp->context_bully.elections_start_time = nl->tick;
    } else {
        for (int neib : neibs) {
            int res = nl->send(dp->node, neib, Message("Elections_coordinator"));
            printf("[%d]: Sent Elections coordinator to [%d] with code %d\n", dp->node, neib, res);
        }
        dp->context_bully.coordinator = dp->node;
    }
    return 1;
}

int workFunction_Elections(Process *dp, Message m) {
    string s = m.getString();
    NetworkLayer *nl = dp->networkLayer;
    set<int> neibs = dp->neibs();
    
    if (!dp->isMyMessage("Elections", s)) {
        return 0;
    }


    // skip mechanism
    if (s == "*TIME") {
        //printf("[%d]: TIME\n", dp->node);
        // 2
        if (dp->context_bully.waiting_for_alive && nl->tick > dp->context_bully.elections_start_time + dp->context_bully.max_waiting_time) {
            dp->context_bully.coordinator = dp->node;
            dp->context_bully.waiting_for_alive = false;
            for (int neib : neibs) {
                int res = nl->send(dp->node, neib, Message("Elections_coordinator"));
                printf("[%d]: Sent Elections coordinator to [%d] with code %d\n", dp->node, neib, res);
            }

            return 1;
        }

        // 3.2
        if (dp->context_bully.waiting_for_coordinator && nl->tick > dp->context_bully.coordinator_check_start_time + dp->context_bully.max_waiting_time) {
            dp->context_bully.waiting_for_coordinator = false;
            init_elections(dp);
            return 1;
        }

        // 4.2
        if (dp->context_bully.start_elections) {
            dp->context_bully.start_elections = false;
            init_elections(dp);
            return 1;
        }

    }

    // 4.1
    if (s == "Elections_init") {
        printf("[%d]: Elections started by [%d]\n", dp->node, m.from);
        if (m.from == -1) {
            dp->context_bully.start_elections = true;
        } else if (dp->node > m.from) {
            int res = nl->send(dp->node, m.from, Message("Elections_alive"));
            printf("[%d]: Sent Elections alive to [%d] with code %d\n", dp->node, m.from, res);
            dp->context_bully.start_elections = true;
        }
        return 1;
    } 

    // 3.1
    if (s == "Elections_alive") {
        printf("[%d]: Elections alive by [%d]\n", dp->node, m.from);
        dp->context_bully.waiting_for_alive = false;
        dp->context_bully.waiting_for_coordinator = true;
        dp->context_bully.coordinator_check_start_time = nl->tick;
        return 1;
    }

    // 6
    if (s == "Elections_coordinator") {
        printf("[%d]: Elections coordinator by [%d]\n", dp->node, m.from);
        dp->context_bully.waiting_for_coordinator = false;
        if (dp->node > m.from) {
            init_elections(dp);
        } else {
            dp->context_bully.coordinator = m.from;
        }
        return 1;
    }
    return 1;
}




int main(int argc, char **argv) {
    string configFile = argc > 1 ? argv[1] : "config.data";
    World w; 
    //w.registerWorkFunction("TEST", workFunction_TEST);
    w.registerWorkFunction("Elections", workFunction_Elections);
    if (w.parseConfig(configFile)) {
        this_thread::sleep_for(chrono::milliseconds(3000000));
	} else {
        printf("can't open file '%s'\n", configFile.c_str());
    }
}

