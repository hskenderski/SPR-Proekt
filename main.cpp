#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <cstring>
#include "Random.cpp"

void *create_shared_memory(size_t size) {
    // Our memory buffer will be readable and writable:
    int protection = PROT_READ | PROT_WRITE;

    // The buffer will be shared (meaning other processes can access it), but
    // anonymous (meaning third-party processes cannot obtain an address for it),
    // so only this process and its children will be able to use it:
    int visibility = MAP_SHARED | MAP_ANONYMOUS;

    // The remaining parameters to `mmap()` are not important for this use case,
    // but the manpage for `mmap` explains their purpose.
    return mmap(nullptr, size, protection, visibility, -1, 0);
}

void *shmem = nullptr;

std::vector<int> points;

std::vector<int> isPlay;

typedef struct arg_struct {
    int i;
} args;

void *random(void *arguments) {
    args *arg = static_cast<args *>(arguments);
    int *arr = (int *) shmem;
    std::random_device random_device; // create object for seeding
    std::mt19937 engine{random_device()}; // create engine and seed it
    std::uniform_int_distribution<> dist(0, 1); // create distribution for integers with [0-1] range

    arr[arg->i] = dist(engine);
    memcpy(shmem, arr, sizeof(&arr));
    sleep(arr[arg->i]);
    free(arguments);
}

enum Coin {
    EZI, TURA
};

static const char *COIN_STRING[] = {
        "ezi", "tura", "don't play"
};


int getPointsForPlayer(int first) {
    if (first == EZI) {
        return 1;
    }
    return 0;
}

void calculatePoints(std::vector<int> newThrow, int peopleNum) {
    std::vector<int> pts(peopleNum, 0);


    for (int i = 0; i < isPlay.size(); i++) {
        if (isPlay[i] == 2) {
            newThrow[i] = 2;
        }
    }

    for (int i = 0; i < peopleNum; i++) {
        if (newThrow[i] == 1) {
            isPlay[i] = 2;
        } else if (newThrow[i] != 2) {
            pts[i] = getPointsForPlayer(newThrow[i]);
            points[i] += pts[i];
        }
    }

    for (int i = 0; i < peopleNum; i++) {
        if (i + 1 == peopleNum) {
            printf("%s", COIN_STRING[newThrow[i]]);
        } else {
            printf("%s - ", COIN_STRING[newThrow[i]]);
        }
    }
    printf("\n");

    for (int i = 0; i < peopleNum; i++) {
        if (i + 1 == peopleNum) {
            printf("%d ", pts[i]);
        } else {
            printf("%d - ", pts[i]);
        }
    }
    printf("\n");

}

void playRound(int peopleNum) {
    pthread_t tid[peopleNum];
    int coin[peopleNum];
    memcpy(shmem, coin, sizeof(coin));

    for (int i = 0; i < peopleNum; i++) {
        if (isPlay[i] != 2) {
            args *arguments = static_cast<args *>(malloc(sizeof(args)));
            arguments->i = i;
            pthread_create(&tid[i], nullptr, random, (void *) arguments);
        }
    }

    for (int i = 0; i < peopleNum; i++) {
        if (isPlay[i] != 2) {
            pthread_join(tid[i], nullptr);
        }

    }

    int *newThrow = (int *) shmem;

    std::vector<int> throws(peopleNum, 0);
    for (int i = 0; i < peopleNum; i++) {
        throws[i] = newThrow[i];
    }

    calculatePoints(throws, peopleNum);
}

int main() {
    int peopleNum = 3;
    printf("Enter the number of the players:");
    scanf("%d", &peopleNum);

    std::vector<int> pointFromAllGame(peopleNum, 0);

    int gameCounter = 1;

    for (;;) {
        printf("Enter the bet of the game:");
        int bet = 10;
        scanf("%d", &bet);

        for (int i = 0; i < peopleNum; i++) {
            if (isPlay.size() == peopleNum && points.size() == peopleNum) {
                break;
            }
            isPlay.push_back(0);
            points.push_back(0);
        }

        shmem = create_shared_memory(500);

        int fg = 0;
        for (;;) {
            if (fg != 0) {
                int ctr = 0;
                for (int i = 0; i < peopleNum; i++) {
                    if (isPlay[i] == 2) {
                        ctr += 1;
                    }
                }

                if (ctr == peopleNum) {
                    int counter1 = 1;
                    for (int i = 0; i < peopleNum - 1; i++) {
                        if (points[i] == points[i + 1]) {
                            counter1++;
                        }
                    }

                    if (counter1 == peopleNum) {
                        for (int i = 0; i < peopleNum; i++) {
                            isPlay[i] = 0;
                        }
                    }

                    int maxPoint = *max_element(points.begin(), points.end());

                    int flag = 0;
                    for (int i = 0; i < peopleNum; i++) {
                        for (int j = i + 1; j < peopleNum; j++) {
                            if (points[i] == points[j] && points[i] == maxPoint) {
                                flag = 1;
                                isPlay[i] = 0;
                                isPlay[j] = 0;
                            }
                        }
                    }

                    if (flag == 0) {
                        break;
                    }

                }
            }
            playRound(peopleNum);
            fg++;
        }

        printf("---Game %d Final Result---\n", gameCounter);

        for (int i = 0; i < points.size(); i++) {
            if (i + 1 == points.size()) {
                printf("%d", points[i]);
            } else {
                printf("%d - ", points[i]);
            }
        }
        printf("\n");

        int maxPoint = *max_element(points.begin(), points.end());

        for (int i = 0; i < peopleNum; i++) {
            if (points[i] == maxPoint) {
                printf("Congratulations player %d win: %d$\n", i + 1, bet * peopleNum);
            }
        }

        for (int i = 0; i < points.size(); i++) {
            pointFromAllGame[i] += points[i];
        }

        printf("Do you wanna play another game?\n");
        printf("Type 0 for NO or 1 for YES\n");

        int choice = 0;
        scanf("%d", &choice);

        if (choice == 0) {
            break;
        }

        for (int i = 0; i < peopleNum; i++) {
            isPlay[i] = 0;
            points[i] = 0;
        }

        gameCounter++;
    }

    printf("The Final Points From All Games!\n");

    for (int i = 0; i < pointFromAllGame.size(); i++) {
        if (i + 1 == pointFromAllGame.size()) {
            printf("%d", pointFromAllGame[i]);
        } else {
            printf("%d - ", pointFromAllGame[i]);
        }
    }

}
