/*
* File: bacon.c
* Author: Jeremiah Creary
* Purpose: calculates the Bacon score of various actors based on the
information given in an input file. 
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Actor {
    char* name;
    struct mlist* movies;
    int visited;
    struct Actor* next;
    struct Actor* parent;
} Actor;

typedef struct mlist {
    struct Movie* m;
    struct mlist* next;
} mlist;

typedef struct Movie {
    char* title;
    struct alist* cast;
    struct Movie* next;
} Movie;

typedef struct alist {
    struct Actor* a;
    struct alist* next;
} alist;

typedef struct Queue {
    Actor* actor;
    Actor* parent;
    int level;
    struct Queue* next;
} Queue;

// Function declarations
void parseMovieFile(const char* filename);
void printList(void* list, int isActorList);
void freeList(void* list, int isActorList);
Actor* findOrAddActor(Actor** actors, const char* name);
void linkActorMovie(Actor* actor, Movie* movie);
void calculateBaconScore(Actor* actors, const char* queryActorName, int lFlag);
void enqueue(Queue** queue, Actor* actor, Actor* parent, int level);
Actor* dequeue(Queue** queue, Actor** parentActor, int* level);
int isQueueEmpty(Queue* queue);
void freeQueue(Queue* queue);
void resetVisited(Actor* actors);
void printPath(Actor* actor);

int errRet = 0;
int lFlag = 0;

// Main function
int main(int argc, char* argv[]) {
    char* inputFileName = NULL;

    // Check for valid number of arguments
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [-l] <input file>\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-l") == 0) {
            lFlag = 1;
        }
        else {
            if (inputFileName) {
                fprintf(stderr, "Multiple input files not allowed\n");
                return 1;
            }
            inputFileName = argv[i];
        }
    }

    // Check if input file is provided
    if (!inputFileName) {
        fprintf(stderr, "Input file is required\n");
        return 1;
    }

    parseMovieFile(inputFileName);

    return errRet;
}

void parseMovieFile(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(1);
    }

    char* line = NULL;
    size_t len = 0;
    long read;
    Movie* movies = NULL;
    Movie* lastMovie = NULL;
    Actor* actors = NULL;

    while ((read = getline(&line, &len, file)) != -1) {
        if (line[read - 1] == '\n') {
            line[read - 1] = 0;
        }

        if (strlen(line) == 0) continue;

        if (strncmp(line, "Movie: ", 7) == 0) {
            // Handle new movie
            Movie* newMovie = malloc(sizeof(Movie));
            newMovie->title = strdup(line + 7); // +7 to skip "Movie: "
            newMovie->cast = NULL;
            newMovie->next = movies;
            movies = newMovie;
            lastMovie = newMovie;
        }
        else {
            // Handle new actor
            Actor* actor = findOrAddActor(&actors, line);
            if (lastMovie) {
                linkActorMovie(actor, lastMovie);
            }
        }
    }

    free(line);
    fclose(file);

    char* query = NULL;
    len = 0;
    while ((read = getline(&query, &len, stdin)) != -1) {
        if (query[read - 1] == '\n') {
            query[read - 1] = '\0';
        }

        if (strcmp(query, "exit") == 0) {
            break;
        }

        calculateBaconScore(actors, query, lFlag);
    }

    free(query);

    freeList(movies, 0);
    freeList(actors, 1);
}

Actor* findOrAddActor(Actor** actors, const char* name) {
    Actor* current = *actors;
    while (current) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }

    // Add new actor
    Actor* newActor = malloc(sizeof(Actor));
    newActor->name = strdup(name);
    newActor->movies = NULL;
    newActor->next = *actors;
    *actors = newActor;
    return newActor;
}

void linkActorMovie(Actor* actor, Movie* movie) {
    // Add movie to actor's mlist
    mlist* newMList = malloc(sizeof(mlist));
    newMList->m = movie;
    newMList->next = actor->movies;
    actor->movies = newMList;

    // Add actor to movie's alist
    alist* newAList = malloc(sizeof(alist));
    newAList->a = actor;
    newAList->next = movie->cast;
    movie->cast = newAList;
}

int actorExists(Actor* actors, const char* name) {
    while (actors != NULL) {
        if (strcmp(actors->name, name) == 0) {
            return 1; // Actor exists
        }
        actors = actors->next;
    }
    return 0; // Actor does not exist
}

void printList(void* list, int isActorList) {
    if (isActorList) {
        Actor* actor = (Actor*)list;
        while (actor) {
            printf("Actor: %s\n", actor->name);
            mlist* movieList = actor->movies;
            while (movieList) {
                printf("\tMovie: %s\n", movieList->m->title);
                movieList = movieList->next;
            }
            actor = actor->next;
        }
    }
    else {
        Movie* movie = (Movie*)list;
        while (movie) {
            printf("Movie: %s\n", movie->title);
            alist* actorList = movie->cast;
            while (actorList) {
                printf("\tActor: %s\n", actorList->a->name);
                actorList = actorList->next;
            }
            movie = movie->next;
        }
    }
}

// Function to free the lists and their sublists
void freeList(void* list, int isActorList) {
    if (isActorList) {
        Actor* actor = (Actor*)list;
        while (actor) {
            Actor* tempActor = actor;
            actor = actor->next;

            mlist* movieList = tempActor->movies;
            while (movieList) {
                mlist* tempMList = movieList;
                movieList = movieList->next;
                free(tempMList);
            }

            free(tempActor->name);
            free(tempActor);
        }
    }
    else {
        Movie* movie = (Movie*)list;
        while (movie) {
            Movie* tempMovie = movie;
            movie = movie->next;

            alist* actorList = tempMovie->cast;
            while (actorList) {
                alist* tempAList = actorList;
                actorList = actorList->next;
                free(tempAList);
            }

            free(tempMovie->title);
            free(tempMovie);
        }
    }
}

void calculateBaconScore(Actor* actors, const char* queryActorName, int lFlag) {
    // Reset visited flags for all actors
    Actor* currentActor = actors;
    while (currentActor != NULL) {
        currentActor->visited = 0;
        currentActor = currentActor->next;
    }

    // Special case for Kevin Bacon
    if (strcmp(queryActorName, "Kevin Bacon") == 0) {
        printf("Score: 0\n");
        if (lFlag == 1) {
            printf("Kevin Bacon\n");
        }
        return;
    }

    // Check if the queried actor exists in the graph
    Actor* queriedActor = actors;
    while (queriedActor != NULL && strcmp(queriedActor->name, queryActorName) != 0) {
        queriedActor = queriedActor->next;
    }
    if (queriedActor == NULL) {
        fprintf(stderr, "No actor named %s entered\n", queryActorName);
        errRet = 1;
        return; // Actor not found
    }

    Queue* queue = NULL;
    currentActor = actors;
    while (currentActor != NULL) {
        currentActor->visited = 0;
        currentActor->parent = NULL;
        currentActor = currentActor->next;
    }

    // Find Kevin Bacon and start BFS from there
    currentActor = actors;
    while (currentActor != NULL && strcmp(currentActor->name, "Kevin Bacon") != 0) {
        currentActor = currentActor->next;
    }

    if (currentActor == NULL) {
        printf("Score: No Bacon!\n");
        return;
    }

    enqueue(&queue, currentActor, NULL, 0);
    currentActor->visited = 1;

    while (!isQueueEmpty(queue)) {
        int currentLevel;
        Actor* parentActor;
        Actor* currentActor = dequeue(&queue, &parentActor, &currentLevel);

        if (strcmp(currentActor->name, queryActorName) == 0) {
            printf("Score: %d\n", currentLevel);
            if (lFlag) {
                printPath(currentActor);
                printf("Kevin Bacon\n");
            }
            freeQueue(queue);
            return;
        }

        mlist* movieList = currentActor->movies;
        while (movieList != NULL) {
            alist* actorList = movieList->m->cast;
            while (actorList != NULL) {
                Actor* linkedActor = actorList->a;
                if (!linkedActor->visited) {
                    linkedActor->visited = 1;
                    linkedActor->parent = currentActor;
                    enqueue(&queue, linkedActor, currentActor, currentLevel + 1);
                }
                actorList = actorList->next;
            }
            movieList = movieList->next;
        }
    }

    printf("Score: No Bacon!\n");
    freeQueue(queue);
}

void enqueue(Queue** queue, Actor* actor, Actor* parent, int level) {
    Queue* newQueueNode = (Queue*)malloc(sizeof(Queue));
    newQueueNode->actor = actor;
    newQueueNode->parent = parent;
    newQueueNode->level = level;
    newQueueNode->next = NULL;

    if (*queue == NULL) {
        *queue = newQueueNode;
    }
    else {
        Queue* temp = *queue;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = newQueueNode;
    }
}

Actor* dequeue(Queue** queue, Actor** parentActor, int* level) {
    if (*queue == NULL) {
        *parentActor = NULL;
        *level = 0;
        return NULL;
    }
    Queue* temp = *queue;
    *queue = (*queue)->next;
    Actor* actor = temp->actor;
    *parentActor = temp->parent;
    *level = temp->level;
    free(temp);
    return actor;
}

int isQueueEmpty(Queue* queue) {
    return queue == NULL;
}

void freeQueue(Queue* queue) {
    while (queue != NULL) {
        Queue* temp = queue;
        queue = queue->next;
        free(temp);
    }
}

void resetVisited(Actor* actors) {
    Actor* current = actors;
    while (current) {
        current->visited = 0;
        current = current->next;
    }
}

void printPath(Actor* actor) {
    // Base case: if this actor is Kevin Bacon or an orphan actor (no parent)
    if (actor == NULL || actor->parent == NULL) {
        return;
    }

    // Find the connecting movie between this actor and their parent
    mlist* movieList = actor->parent->movies;
    while (movieList != NULL) {
        alist* actorList = movieList->m->cast;
        while (actorList != NULL) {
            if (actorList->a == actor) {
                printf("%s\nwas in %s with\n", actor->name, movieList->m->title);
                printPath(actor->parent);
                return; // Exit after printing the connecting movie
            }
            actorList = actorList->next;
        }
        movieList = movieList->next;
    }
}