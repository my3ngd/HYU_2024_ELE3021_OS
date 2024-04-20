#include "types.h"
#include "stat.h"
#include "user.h"

#define RN_STUDENT_ID 2020098240

int mf = 0;

void level_cnt_init(int* level_cnt) {
    level_cnt[0] = level_cnt[1] = level_cnt[2] = level_cnt[3] = level_cnt[4] = 0;
}

void add_level_cnt(int* level_cnt) {
    int lev;

    lev = getlev();
    if (lev == 99) ++level_cnt[4];
    else ++level_cnt[lev];
}

void print_level_cnt(int* level_cnt) {
    printf(1, " [ process %d ]\n", getpid());
    printf(1, "\t- L0:\t\t%d\n", level_cnt[0]);
    printf(1, "\t- L1:\t\t%d\n", level_cnt[1]);
    printf(1, "\t- L2:\t\t%d\n", level_cnt[2]);
    printf(1, "\t- L3:\t\t%d\n", level_cnt[3]);
    printf(1, "\t- MONOPOLIZED:\t%d\n\n", level_cnt[4]);
}

void rn_test1() {
    int p, p1, p2, p3, w_cnt, w_list[3], level_cnt[5];

    printf(1, "rn_test1 [L0 Only]\n");

    level_cnt_init(level_cnt);
    p1 = fork();
    if (p1 == 0) {
        for (int cnt = 1000; cnt--; ) {
            rn_sleep(3);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt);
        exit();
        // wait();
    }

    p2 = fork();
    if (p2 == 0) {
        for (int cnt = 500; cnt--; ) {
            rn_sleep(3);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt);
        exit();
        // wait();
    }

    p3 = fork();
    if (p3 == 0) {
        for (int cnt = 100; cnt--; ) {
            rn_sleep(3);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt);
        exit();
        // wait();
    }

    w_list[0] = p3;
    w_list[1] = p2;
    w_list[2] = p1;
    w_cnt = 0;
    for (; w_cnt < 3; ) {
        p = wait();
        // w_cnt++;
        // continue;
        if (p == w_list[w_cnt]) ++w_cnt;
        else {
            printf(1, "rn_test1 failed\n expected wait pid = %d, but wait pid = %d\n", w_list[w_cnt], p);
            exit();
        }
    }

    printf(1, "rn_test1 finished successfully\n\n\n");
}

void rn_test2() {
    int p, p1, p2, p3, p4, w_cnt, w_list[4], level_cnt[5];

    printf(1, "rn_test2 [L1~L2 Only]\n");

    level_cnt_init(level_cnt);
    p1 = fork();
    if (p1 == 0) {
        for (int cnt = 300; cnt--; ) {
            rn_sleep(20);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt);
        exit();
        wait();
    }

    p2 = fork();
    if (p2 == 0) {
        for (int cnt = 200; cnt--; ) {
            rn_sleep(20);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt);
        exit();
        wait();
    }

    p3 = fork();
    if (p3 == 0) {
        for (int cnt = 100; cnt--; ) {
            rn_sleep(20);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt);
        exit();
        wait();
    }

    p4 = fork();
    if (p4 == 0) {
        for (int cnt = 50; cnt--; ) {
            rn_sleep(20);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt);
        exit();
        wait();
    }

    if (p4 % 2 == 1) {
        w_list[0] = p4;
        w_list[1] = p2;
        w_list[2] = p3;
        w_list[3] = p1;
    } else {
        w_list[0] = p3;
        w_list[1] = p1;
        w_list[2] = p4;
        w_list[3] = p2;
    }
    w_cnt = 0;
    for (; w_cnt < 4; ) {
        p = wait();
        w_cnt++;
        continue;
        if (p == w_list[w_cnt]) ++w_cnt;
        else {
            printf(1, "rn_test2 failed\n expected wait pid = %d, but wait pid = %d\n", w_list[w_cnt], p);
            exit();
        }
    }

    printf(1, "rn_test2 finished successfully\n\n\n");
}

void rn_test3() {
    int p, p1, p2, p3, p4, w_cnt, w_list[4], level_cnt[5];

    printf(1, "rn_test3 [L3 Only]\n");

    level_cnt_init(level_cnt);
    p1 = fork();
    if (p1 == 0) {
        for (int cnt = 60; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
            if (setpriority(getpid(), 1) < 0) printf(1, "setpriority failed\n");
            yield();
        }
        print_level_cnt(level_cnt);
        exit();
    }

    p2 = fork();
    if (p2 == 0) {
        for (int cnt = 60; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
            if (setpriority(getpid(), 10) < 0) printf(1, "setpriority failed\n");
            yield();
        }
        print_level_cnt(level_cnt);
        exit();
    }

    p3 = fork();
    if (p3 == 0) {
        for (int cnt = 60; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
            if (setpriority(getpid(), 4) < 0) printf(1, "setpriority failed\n");
            yield();
        }
        print_level_cnt(level_cnt);
        exit();
    }

    p4 = fork();
    if (p4 == 0) {
        for (int cnt = 60; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
            if (setpriority(getpid(), 4) < 0) printf(1, "setpriority failed\n");
            yield();
        }
        print_level_cnt(level_cnt);
        exit();
    }

    w_list[0] = p2;
    w_list[1] = p4;
    w_list[2] = p3;
    w_list[3] = p1;
    w_cnt = 0;
    for (; w_cnt < 4; ) {
        p = wait();
        if (p == w_list[w_cnt]) ++w_cnt;
        else {
            printf(1, "rn_test3 failed\n expected wait pid = %d, but wait pid = %d\n", w_list[w_cnt], p);
            exit();
        }
    }

    printf(1, "rn_test3 finished successfully\n\n\n");
}

void rn_test4() {
    int p, p1, p2, p3, w_cnt, w_list[3], level_cnt[5];

    printf(1, "rn_test4 [MoQ Only - wait 5sec]\n");

    level_cnt_init(level_cnt);
    
    p1 = fork();
    if (p1 == 0) {
        mf = setmonopoly(getpid(), RN_STUDENT_ID);
        if (mf < 0) printf(1, "setmonopoly failed\n");
        printf(1, "mf val = %d\n", mf);
        rn_sleep(1000);
        printf(1, "monopolized process %d\n", getpid());
        for (int cnt = 100; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
        }
        print_level_cnt(level_cnt);
        exit();
    }

    p2 = fork();
    if (p2 == 0) {
        mf = setmonopoly(getpid(), RN_STUDENT_ID);
        if (mf < 0) printf(1, "setmonopoly failed\n");
        printf(1, "mf val = %d\n", mf);
        rn_sleep(1000);
        printf(1, "monopolized process %d\n", getpid());
        for (int cnt = 60; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
        }
        print_level_cnt(level_cnt);
        exit();
    }

    p3 = fork();
    if (p3 == 0) {
        mf = setmonopoly(getpid(), RN_STUDENT_ID);
        if (mf < 0) printf(1, "setmonopoly failed\n");
        printf(1, "mf val = %d\n", mf);
        rn_sleep(1000);
        printf(1, "monopolized process %d\n", getpid());
        for (int cnt = 30; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
        }
        print_level_cnt(level_cnt);
        exit();
    }

    rn_sleep(5000);
    printf(1, "monopolize!\n");
    monopolize();

    w_list[0] = p1;
    w_list[1] = p2;
    w_list[2] = p3;
    w_cnt = 0;
    for (; w_cnt < 3; ) {
        p = wait();
        if (p == w_list[w_cnt]) ++w_cnt;
        else {
            printf(1, "rn_test4 failed\n expected wait pid = %d, but wait pid = %d\n", w_list[w_cnt], p);
            exit();
        }
    }

    printf(1, "rn_test4 finished successfully\n\n\n");
}

void rn_test5() {
    int p, p1, p2, p3, p4, p5, p6, w_cnt, w_list[6], level_cnt[5];

    printf(1, "rn_test5 [L0, L1, L2, L3]\n");

    level_cnt_init(level_cnt);
    p1 = fork();
    if (p1 == 0) {
        for (int cnt = 60; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
            if (setpriority(getpid(), 10) < 0) printf(1, "setpriority failed\n");
            yield();
        }
        print_level_cnt(level_cnt);
        exit();
    }

    p2 = fork();
    if (p2 == 0) {
        for (int cnt = 60; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
            if (setpriority(getpid(), 4) < 0) printf(1, "setpriority failed\n");
            yield();
        }
        print_level_cnt(level_cnt);
        exit();
    }

    p3 = fork();
    if (p3 == 0) {
        for (int cnt = 200; cnt--; ) {
            rn_sleep(20);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt);
        exit();
    }

    p4 = fork();
    if (p4 == 0) {
        for (int cnt = 200; cnt--; ) {
            rn_sleep(20);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt);
        exit();
    }

    p5 = fork();
    if (p5 == 0) {
        for (int cnt = 1000; cnt--; ) {
            rn_sleep(3);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt);
        exit();
    }

    p6 = fork();
    if (p6 == 0) {
        for (int cnt = 500; cnt--; ) {
            rn_sleep(3);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt);
        exit();
    }

    w_list[0] = p6;
    w_list[1] = p5;
    if (p3 % 2 == 1) {
        w_list[2] = p3;
        w_list[3] = p4;
    } else {
        w_list[2] = p4;
        w_list[3] = p3;
    }
    w_list[4] = p1;
    w_list[5] = p2;
    w_cnt = 0;
    for (; w_cnt < 6; ) {
        p = wait();
        if (p == w_list[w_cnt]) ++w_cnt;
        else {
            printf(1, "rn_test5 failed\n expected wait pid = %d, but wait pid = %d\n", w_list[w_cnt], p);
            exit();
        }
    }

    printf(1, "rn_test5 finished successfully\n\n\n");
}

void rn_test6() {
    int p, p1, p2, p3, p4, p5, p6, p7, w_cnt, w_list[7], level_cnt[5];

    printf(1, "rn_test6 [L0, L1, L2, L3, MoQ]\n");

    level_cnt_init(level_cnt);
    p1 = fork();
    if (p1 == 0) {
        for (int cnt = 60; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
            if (setpriority(getpid(), 10) < 0) printf(1, "setpriority failed\n");
            yield();
        }
        print_level_cnt(level_cnt);
        exit();
    }

    p2 = fork();
    if (p2 == 0) {
        for (int cnt = 60; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
            if (setpriority(getpid(), 4) < 0) printf(1, "setpriority failed\n");
            yield();
        }
        print_level_cnt(level_cnt);
        exit();
    }

    p3 = fork();
    if (p3 == 0) {
        for (int cnt = 200; cnt--; ) {
            rn_sleep(20);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt);
        exit();
    }

    p4 = fork();
    if (p4 == 0) {
        for (int cnt = 200; cnt--; ) {
            rn_sleep(20);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt);
        exit();
    }

    p5 = fork();
    if (p5 == 0) {
        for (int cnt = 1000; cnt--; ) {
            rn_sleep(3);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt);
        exit();
    }

    p6 = fork();
    if (p6 == 0) {
        for (int cnt = 500; cnt--; ) {
            rn_sleep(3);
            add_level_cnt(level_cnt);
            yield();
        }
        print_level_cnt(level_cnt);
        exit();
    }

    p7 = fork();
    if (p7 == 0) {
        mf = setmonopoly(getpid(), RN_STUDENT_ID);
        if (mf < 0) printf(1, "setmonopoly failed\n");
        printf(1, "mf val = %d\n", mf);
        rn_sleep(1000);
        printf(1, "monopolized process %d\n", getpid());
        for (int cnt = 30; cnt--; ) {
            rn_sleep(60);
            add_level_cnt(level_cnt);
        }
        print_level_cnt(level_cnt);
        exit();
    }

    w_list[0] = p6;
    w_list[1] = p5;
    if (p3 % 2 == 1) {
        w_list[2] = p3;
        w_list[4] = p4;
    } else {
        w_list[2] = p4;
        w_list[4] = p3;
    }
    w_list[3] = p7;
    w_list[5] = p1;
    w_list[6] = p2;
    w_cnt = 0;
    for (; w_cnt < 7; ) {
        p = wait();
        if (p == w_list[w_cnt]) ++w_cnt;
        else {
            printf(1, "rn_test6 failed\n expected wait pid = %d, but wait pid = %d\n", w_list[w_cnt], p);
            exit();
        }
        if (w_cnt == 3) {
            printf(1, "monopolize!\n");
            monopolize();
        }
    }

    printf(1, "rn_test6 finished successfully\n\n\n");
}

int main(int argc, char** argv) {
    rn_test1();  // 실패
    // rn_test2();  // 실패
    // rn_test3();  // 통과
    // rn_test4();  // 통과
    // rn_test5();  // 실패
    // rn_test6();  // 실패

    exit();
}

/*
expected output

rn_test1 [L0 Only]
 [ process 6 ]
	- L0:		100
	- L1:		0
	- L2:		0
	- L3:		0
	- MONOPOLIZED:	0

 [ process 5 ]
	- L0:		500
	- L1:		0
	- L2:		0
	- L3:		0
	- MONOPOLIZED:	0

 [ process 4 ]
	- L0:		1000
	- L1:		0
	- L2:		0
	- L3:		0
	- MONOPOLIZED:	0

rn_test1 finished successfully


rn_test2 [L1~L2 Only]
 [ process 9 ]
	- L0:		0
	- L1:		100
	- L2:		0
	- L3:		0
	- MONOPOLIZED:	0

 [ process 7 ]
	- L0:		8
	- L1:		292
	- L2:		0
	- L3:		0
	- MONOPOLIZED:	0

 [ process 10 ]
	- L0:		9
	- L1:		0
	- L2:		41
	- L3:		0
	- MONOPOLIZED:	0

 [ process 8 ]
	- L0:		12
	- L1:		0
	- L2:		188
	- L3:		0
	- MONOPOLIZED:	0

rn_test2 finished successfully


rn_test3 [L3 Only]
 [ process 12 ]
	- L0:		1
	- L1:		0
	- L2:		6
	- L3:		53
	- MONOPOLIZED:	0

 [ process 14 ]
	- L0:		5
	- L1:		0
	- L2:		12
	- L3:		43
	- MONOPOLIZED:	0

 [ process 13 ]
	- L0:		7
	- L1:		2
	- L2:		0
	- L3:		51
	- MONOPOLIZED:	0

 [ process 11 ]
	- L0:		1
	- L1:		14
	- L2:		0
	- L3:		45
	- MONOPOLIZED:	0

rn_test3 finished successfully


rn_test4 [MoQ Only - wait 5sec]
monopolize!
monopolized process 15
 [ process 15 ]
	- L0:		0
	- L1:		0
	- L2:		0
	- L3:		0
	- MONOPOLIZED:	100

monopolized process 16
 [ process 16 ]
	- L0:		0
	- L1:		0
	- L2:		0
	- L3:		0
	- MONOPOLIZED:	60

monopolized process 17
 [ process 17 ]
	- L0:		0
	- L1:		0
	- L2:		0
	- L3:		0
	- MONOPOLIZED:	30

rn_test4 finished successfully


rn_test5 [L0, L1, L2, L3]
 [ process 23 ]
	- L0:		500
	- L1:		0
	- L2:		0
	- L3:		0
	- MONOPOLIZED:	0

 [ process 22 ]
	- L0:		1000
	- L1:		0
	- L2:		0
	- L3:		0
	- MONOPOLIZED:	0

 [ process 21 ]
	- L0:		9
	- L1:		191
	- L2:		0
	- L3:		0
	- MONOPOLIZED:	0

 [ process 20 ]
	- L0:		14
	- L1:		0
	- L2:		186
	- L3:		0
	- MONOPOLIZED:	0

 [ process 18 ]
	- L0:		9
	- L1:		0
	- L2:		9
	- L3:		42
	- MONOPOLIZED:	0

 [ process 19 ]
	- L0:		2
	- L1:		14
	- L2:		0
	- L3:		44
	- MONOPOLIZED:	0

rn_test5 finished successfully


rn_test6 [L0, L1, L2, L3, MoQ]
 [ process 29 ]
	- L0:		500
	- L1:		0
	- L2:		0
	- L3:		0
	- MONOPOLIZED:	0

 [ process 28 ]
	- L0:		1000
	- L1:		0
	- L2:		0
	- L3:		0
	- MONOPOLIZED:	0

 [ process 27 ]
	- L0:		10
	- L1:		190
	- L2:		0
	- L3:		0
	- MONOPOLIZED:	0

monopolize!
monopolized process 30
 [ process 30 ]
	- L0:		0
	- L1:		0
	- L2:		0
	- L3:		0
	- MONOPOLIZED:	30

 [ process 26 ]
	- L0:		14
	- L1:		0
	- L2:		186
	- L3:		0
	- MONOPOLIZED:	0

 [ process 24 ]
	- L0:		8
	- L1:		0
	- L2:		8
	- L3:		44
	- MONOPOLIZED:	0

 [ process 25 ]
	- L0:		13
	- L1:		3
	- L2:		0
	- L3:		44
	- MONOPOLIZED:	0

rn_test6 finished successfully

*/
