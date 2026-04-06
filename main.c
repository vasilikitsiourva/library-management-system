#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS

#pragma warning(disable : 4996)
#endif

#include "library.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define REC_HEAP_CAPACITY 64
int SLOTS = 0; 
static int DISPLAY_READY = 0;
library_t library; 

bool registerGenre(library_t *lib, int gid, const char *name);
bool registerBook(library_t *lib, int bid, int gid, const char *title);
member_t *findMember(member_t **head, int sid);
book_t *findBook(library_t *lib, int bid);
genre_t *findeGenre(library_t *lib, int gid);
void insertMember(library_t *lib, int sid, const char *name);
void registerLoan(library_t *lib, int sid, int bid);
void returnBook(library_t *lib, int sid, int bid, const char *score, const char *status);
void repositionBook(genre_t *g, book_t *b);
void Slots(library_t *lib, int slots);
void commandD(library_t *lib);

/* Printing helpers */
void printPG(library_t *lib, int gid);
void printPM(library_t *lib, int sid);
void printPD(library_t *lib);
void printPS(library_t *lib);
static long long compute_genre_points(genre_t *g);
void debugPrintAllBooks(const library_t *lib);
void debugPrintGenreBooks(const genre_t *g);

/* Helper list for D */
helper_t *create(long long data);
void insert(helper_t **head, long long data);
helper_t *head = NULL;

/*  AVL for book index (sorted by title) */
int getHeight(bookIndex_t *node);
int updateHeight(bookIndex_t *node);
int getBalance(bookIndex_t *node);
bookIndex_t *rotate_left(bookIndex_t *root);
bookIndex_t *rotate_right(bookIndex_t *root);
bookIndex_t *createBookIndexNode(book_t *book);
bookIndex_t *avl_insert(bookIndex_t *root, book_t *book);
bookIndex_t *avl_search(bookIndex_t *root, const char *title);
bookIndex_t *avl_find_min(bookIndex_t *root);
bookIndex_t *avl_delete(bookIndex_t *root, const char *title);

/*Recommendation Heap */
static int compareBooks(const book_t *a, const book_t *b);
RecHeap_t *recheapcrete(int capacity);
static void swapBooks(RecHeap_t *heap, int i, int j);
static void HeapSiftDown(RecHeap_t *heap, int index);
static void HeapSiftUp(RecHeap_t *heap, int index);
void UpdateRecommendationHeap(RecHeap_t *heap, book_t *book);
static int WorstBookIndex(RecHeap_t *heap);
static void HeapRemoveIndex(RecHeap_t *heap, int index);
void commandTOP(library_t *lib, int k);
void commandAM(library_t *lib);
void commandX(library_t *lib);
void commandU(library_t *lib, int bid, const char *new_title);
void freeBooks(book_t *book);
void freeGenres(genre_t *genre);
void freeMembers(member_t *member);
void freeLoans(loan_t *loan);
void freeAVL(bookIndex_t *root);
void freeMemberActivities(MemberActivity_t *activity);
void freeRecommendationHeap(RecHeap_t *heap);
void commandBF(library_t *lib);
int is_better_activity(MemberActivity_t *a, MemberActivity_t *b);
void repositionActivity(library_t *lib, MemberActivity_t *act);
static int findMinLeafIndex(RecHeap_t *heap);

/*unsorted list*/
MemberActivity_t *createMemberActivity(library_t *lib, int sid);
void commandAM(library_t *lib);

int main(int argc, char *argv[])
{
    /* Initialize library */
    library.genres = NULL;
    library.members = NULL;
    library.genre_count = 0;
    library.member_count = 0;
    library.num_active_loans = 0;
    library.num_books = 0;
    library.num_members = 0;
    library.total_review_score = 0;
    library.total_reviews_count = 0;
    library.recommendations = recheapcrete(REC_HEAP_CAPACITY); // NULL = recheapcrete(REC_HEAP_CAPACITY);
    library.activity = NULL;
    FILE *ptr = NULL;
    if (argc >= 2)
    {
        ptr = fopen(argv[1], "r");
        if (!ptr)
        {
            perror("fopen");
            return 1;
        }
    }
    else
    {
        ptr = stdin;
    }

    char line[1024];

    while (fgets(line, sizeof(line), ptr))
    {
        /* skip leading whitespace */
        char *p = line;
        while (*p == ' ' || *p == '\t')
            p++;

        /* skip empty lines */
        if (*p == '\0' || *p == '\n')
            continue;

        /* skip comments */
        if (*p == '#')
            continue;

        /* Multi-letter print commands first: PG, PM, PD, PS */

        /* PG <gid> */
        if (p[0] == 'P' && p[1] == 'G')
        {
            int gid;
            if (sscanf(p, "PG %d", &gid) == 1)
                printPG(&library, gid);
            else
                printf("IGNORED\n");
            continue;
        }

        /* PM <sid> */
        if (p[0] == 'P' && p[1] == 'M')
        {
            int sid;
            if (sscanf(p, "PM %d", &sid) == 1)
                printPM(&library, sid);
            else
                printf("IGNORED\n");
            continue;
        }

        /* PD */
        if (strncmp(p, "PD", 2) == 0)
        {
            printPD(&library);
            continue;
        }

        /* PS */
        if (strncmp(p, "PS", 2) == 0)
        {
            printPS(&library);
            continue;
        }

        /* Debug */
        if (strncmp(p, "Debug", 5) == 0)
        {
            debugPrintAllBooks(&library);
            continue;
        }

        /* Single-letter commands */
        char command;
        if (sscanf(p, " %c", &command) != 1)
            continue;

        if (command == 'G')
        {
            int gid;
            char name[NAME_MAX];
            if (sscanf(p, "G %d %63[^\n]", &gid, name) == 2)
            {
                if (registerGenre(&library, gid, name))
                    printf("DONE\n");
                else
                    printf("IGNORED\n");
            }
            else
            {
                printf("IGNORED\n");
            }
        }
        else if (command == 'S')
        {
            int s;
            if (sscanf(p, "S %d", &s) == 1)
                Slots(&library, s);
            else
                printf("IGNORED\n");
        }
        /* BK <bid> <gid> <title...> */
        else if (p[0] == 'B' && p[1] == 'K')
        {
            int bid, gid;
            char title[TITLE_MAX] = {0};

            if (sscanf(line, "BK %d %d %127[^\n]", &bid, &gid, title) == 3)
            {
                if (registerBook(&library, bid, gid, title))
                    printf("DONE\n");
                else
                    printf("IGNORED\n");
            }
            else
            {
                printf("IGNORED\n");
            }
        }
        else if (command == 'M')
        {
            int sid;
            char name[NAME_MAX];
            if (sscanf(p, "M %d %63[^\n]", &sid, name) >= 1)
            {
                insertMember(&library, sid, name);
            }
            else
            {
                printf("IGNORED\n");
            }
        }
        else if (command == 'L')
        {
            /* ΠΡΟΣΘΗΚΗ \r και \t */
            char *cmd = strtok(line, " \t\r\n");
            if (cmd && strcmp(cmd, "L") == 0)
            {
                char *s_sid = strtok(NULL, " \t\r\n");
                char *s_bid = strtok(NULL, " \t\r\n");
                if (s_sid && s_bid)
                {
                    int sid = atoi(s_sid);
                    int bid = atoi(s_bid);
                    registerLoan(&library, sid, bid);
                }
                else
                {
                    printf("IGNORED\n");
                }
            }
            else
            {
                printf("IGNORED\n");
            }
        }
        /* R <sid> <bid> ok <score|NA>
           R <sid> <bid> <score|NA> ok
           R <sid> <bid> lost */
        else if (command == 'R')
        {
            /* ΠΡΟΣΘΗΚΗ \r και \t */
            char *cmd = strtok(line, " \t\r\n");
            if (cmd && strcmp(cmd, "R") == 0)
            {
                char *s_sid = strtok(NULL, " \t\r\n");
                char *s_bid = strtok(NULL, " \t\r\n");
                char *t3 = strtok(NULL, " \t\r\n");
                char *t4 = strtok(NULL, " \t\r\n");

                if (!s_sid || !s_bid || !t3)
                {
                    printf("IGNORED\n");
                }
                else
                {
                    int sid = atoi(s_sid);
                    int bid = atoi(s_bid);

                    if (strcmp(t3, "lost") == 0)
                    {
                        returnBook(&library, sid, bid, "NA", "lost");
                    }
                    else if (strcmp(t3, "ok") == 0)
                    {
                        const char *scoreArg = (t4 ? t4 : "NA");
                        returnBook(&library, sid, bid, scoreArg, "ok");
                    }
                    else
                    {
                        if (t4 && strcmp(t4, "ok") == 0)
                        {
                            returnBook(&library, sid, bid, t3, "ok");
                        }
                        else
                        {
                            printf("IGNORED\n");
                        }
                    }
                }
            }
            else
            {
                printf("IGNORED\n");
            }
        }
        else if (command == 'D')

        {

            /* include \r so "D\r" (CRLF files) is recognized */

            char *cmd = strtok(line, " \t\r\n");

            if (cmd && strcmp(cmd, "D") == 0)

            {

                commandD(&library);
            }

            else

            {

                printf("IGNORED\n");
            }
        }

        else if (command == 'F')
        {
            char title[TITLE_MAX];
            /* ΠΡΟΣΟΧΗ: Εδώ ανοίγει άγκιστρο { */
            if (sscanf(p, "F %127[^\n]", title) == 1)
            {
                char *cr = strchr(title, '\r');
                if (cr)
                    *cr = '\0';

                /* FIX: Search in ALL genres */
                int found = 0;
                for (genre_t *g = library.genres; g; g = g->next)
                {
                    bookIndex_t *node = avl_search(g->bookIndex, title);
                    if (node)
                    {
                        printf("F \"%s\" BID=%d AVG=%d\n", node->book->title, node->book->bid, node->book->avg);
                        found = 1;
                        break;
                    }
                }
                if (!found)
                    printf("F \"%s\" NOT FOUND\n", title);
            } /* Εδώ κλείνει το άγκιστρο } */
            else
            {
                printf("IGNORED\n");
            }
        }

        else if (command == 'D')

        {

            /* include \r so "D\r" (CRLF files) is recognized */

            char *cmd = strtok(line, " \t\r\n");

            if (cmd && strcmp(cmd, "D") == 0)

            {

                commandD(&library);
            }

            else

            {

                printf("IGNORED\n");
            }
        }
        else if (strncmp(p, "TOP", 3) == 0)
        {
            int k;
            
            if (sscanf(p, "TOP %d", &k) == 1)
            {
                commandTOP(&library, k);
            }
            else
            {
                printf("IGNORED\n");
            }
            continue;
        }

        else if (strncmp(p, "AM", 2) == 0)
        {
            commandAM(&library);
            continue;
        }
        else if (p[0] == 'U')
        {
            int bid;
            char newTitle[TITLE_MAX];

          
            if (sscanf(p, "U %d %127[^\n]", &bid, newTitle) == 2)
            {
                /* --- ΠΡΟΣΘΗΚΗ: Αφαίρεση \r --- */
                char *cr = strchr(newTitle, '\r');
                if (cr)
                    *cr = '\0';
                /* ----------------------------- */

                commandU(&library, bid, newTitle);
            }
            else
            {
                printf("IGNORED\n");
            }

            continue;
        }
        else if (p[0] == 'X')
        {
            commandX(&library);
            continue;
        }

        else if (strncmp(p, "BF", 2) == 0)
        {
            commandBF(&library);
            continue;
        }

        else
        {
            printf("Unknown command: %c\n", command);
        }
    }

    if (ptr != stdin)
        fclose(ptr);
    return 0;
}

bool registerGenre(library_t *lib, int gid, const char *name)
{
    if (!lib || !name)
        return false;

    genre_t *curr = lib->genres, *prev = NULL;
    while (curr)
    {
        if (curr->gid == gid)
            return false; /* duplicate */
        if (curr->gid > gid)
            break;
        prev = curr;
        curr = curr->next;
    }

    genre_t *newg = malloc(sizeof(genre_t));
    if (!newg)
        return false;

    newg->gid = gid;
    strncpy(newg->name, name, NAME_MAX);
    newg->name[NAME_MAX - 1] = '\0';
    char *cr = strchr(newg->name, '\r');
    if (cr)
        *cr = '\0';
    newg->books = NULL;
    newg->lost_count = 0;
    newg->invalid_count = 0;
    newg->slots = 0;
    newg->display = NULL;
    newg->bookIndex = NULL;
    newg->next = curr;

    if (prev)
        prev->next = newg;
    else
        lib->genres = newg;

    lib->genre_count++;
    return true;
}

genre_t *findeGenre(library_t *lib, int gid)
{
    genre_t *g = lib->genres;
    while (g)
    {
        if (g->gid == gid)
            return g;
        g = g->next;
    }
    return NULL;
}

book_t *findBook(library_t *lib, int bid)
{
    for (genre_t *g = lib->genres; g; g = g->next)
    {
        for (book_t *b = g->books; b; b = b->next)
        {
            if (b->bid == bid)
                return b;
        }
    }
    return NULL;
}

bool registerBook(library_t *lib, int bid, int gid, const char *title)
{
    if (!lib || !title)
        return false;

    genre_t *genre = findeGenre(lib, gid);
    if (!genre)
        return false;

    /* check duplicate bid across all genres */
    for (genre_t *g = lib->genres; g; g = g->next)
    {
        for (book_t *b = g->books; b; b = b->next)
        {
            if (b->bid == bid)
                return false;
        }
    }

    book_t *newb = malloc(sizeof(book_t));
    if (!newb)
        return false;

    newb->bid = bid;
    newb->gid = gid;
    strncpy(newb->title, title, TITLE_MAX);
    newb->title[TITLE_MAX - 1] = '\0';
    char *cr = strchr(newb->title, '\r');
    if (cr)
        *cr = '\0';
    newb->sum_scores = 0;
    newb->is_loaned = 0;
    newb->n_reviews = 0;
    newb->avg = 0;
    newb->lost_flag = 0;
    newb->prev = newb->next = NULL;
    newb->heap_pos = -1;

    /* insert in genre list: descending avg, tie smaller bid first */
    book_t *prev = NULL, *curr = genre->books;
    while (curr && (curr->avg > newb->avg ||
                    (curr->avg == newb->avg && curr->bid < newb->bid)))
    {
        prev = curr;
        curr = curr->next;
    }

    newb->next = curr;
    newb->prev = prev;
    if (prev)
        prev->next = newb;
    else
        genre->books = newb;
    if (curr)
        curr->prev = newb;

    /* insert in AVL index by title */
    genre->bookIndex = avl_insert(genre->bookIndex, newb);
    lib->num_books++;

    return true;
}

/*  Member & loan handling  */

member_t *findMember(member_t **head, int sid)
{
    if (!head)
        return NULL;
    member_t *curr = *head;
    while (curr)
    {
        if (curr->sid == sid)
            return curr;
        curr = curr->next;
    }
    return NULL;
}

void insertMember(library_t *lib, int sid, const char *name)
{
    if (!lib || !name)
        return;

    if (findMember(&lib->members, sid))
    {
        printf("IGNORED\n");
        return;
    }

    member_t *newm = malloc(sizeof(member_t));
    if (!newm)
        return;

    newm->sid = sid;
    strncpy(newm->name, name, NAME_MAX);
    newm->name[NAME_MAX - 1] = '\0';
    char *cr = strchr(newm->name, '\r');
    if (cr)
        *cr = '\0';

    newm->loans = malloc(sizeof(loan_t));
    newm->loans->next = NULL;
    newm->activity = NULL;

    member_t *prev = NULL, *curr = lib->members;
    while (curr && curr->sid < sid)
    {
        prev = curr;
        curr = curr->next;
    }

    newm->next = curr;
    if (prev)
        prev->next = newm;
    else
        lib->members = newm;

    lib->member_count++;
    lib->num_members++;

    printf("DONE\n");
}

void registerLoan(library_t *lib, int sid, int bid)
{
    member_t *member = findMember(&lib->members, sid);
    book_t *book = findBook(lib, bid);

    if (!member || !book || book->is_loaned)
    {
        printf("IGNORED\n");
        return;
    }

    /* check if user already has this book */
    for (loan_t *c = member->loans->next; c; c = c->next)
    {
        if (c->bid == bid)
        {
            printf("IGNORED\n");
            return;
        }
    }

    /* insert new loan */
    loan_t *newloan = malloc(sizeof(loan_t));
    newloan->sid = sid;
    newloan->bid = bid;
    newloan->next = member->loans->next;
    member->loans->next = newloan;

    printf("DONE\n");

    /* update MemberActivity ONCE */
    MemberActivity_t *act = createMemberActivity(lib, sid);
    act->loans_count++;
    repositionActivity(lib, act);

    book->is_loaned = 1;
    lib->num_active_loans++;
}

void repositionBook(genre_t *g, book_t *b)
{
    /* remove from current position */
    if (b->prev)
        b->prev->next = b->next;
    else
        g->books = b->next;

    if (b->next)
        b->next->prev = b->prev;

    b->prev = b->next = NULL;

    /* reinsert in descending avg, tie smaller bid */
    book_t *curr = g->books, *prev = NULL;
    while (curr)
    {
        if (b->avg > curr->avg)
            break;
        if (b->avg == curr->avg && b->bid < curr->bid)
            break;
        prev = curr;
        curr = curr->next;
    }

    b->next = curr;
    b->prev = prev;
    if (prev)
        prev->next = b;
    else
        g->books = b;
    if (curr)
        curr->prev = b;
}

void returnBook(library_t *lib, int sid, int bid, const char *score, const char *status)
{
    member_t *m = findMember(&lib->members, sid);
    book_t *b = findBook(lib, bid);

    if (!m || !b)
    {
        printf("IGNORED\n");
        return;
    }

    /* find loan */
    loan_t *prev = m->loans;
    loan_t *cur = prev->next;

    while (cur && cur->bid != bid)
    {
        prev = cur;
        cur = cur->next;
    }

    if (!cur)
    {
        printf("IGNORED\n");
        return;
    }

    /* remove loan */
    prev->next = cur->next;
    free(cur);
    b->is_loaned = 0;
    lib->num_active_loans--;

    /* LOST CASE */
    if (strcmp(status, "lost") == 0)
    {
        b->lost_flag = 1;
        UpdateRecommendationHeap(lib->recommendations, b);
        printf("DONE\n");
        return;
    }

    /* OK but NA score  no review */
    if (strcmp(status, "ok") == 0 && strcmp(score, "NA") == 0)
    {
        printf("DONE\n");
        return;
    }

    /* OK + score */
    if (strcmp(status, "ok") == 0)
    {
        int s = atoi(score);
        if (s < 0 || s > 10)
        {
            printf("IGNORED\n");
            return;
        }

        /* update book stats */
        b->sum_scores += s;
        b->n_reviews += 1;
        lib->total_review_score += s;
        lib->total_reviews_count += 1;
        b->avg = b->sum_scores / b->n_reviews;

        /* reposition inside genre list */
        genre_t *g = findeGenre(lib, b->gid);
        if (g)
            repositionBook(g, b);

        /* update MemberActivity exactly ONCE */
        MemberActivity_t *act = createMemberActivity(lib, sid);
        act->reviews_count++;
        act->score_sum += s;
        repositionActivity(lib, act);

        /* update Recommendation Heap */
        UpdateRecommendationHeap(lib->recommendations, b);

        printf("DONE\n");
        return;
    }

    printf("IGNORED\n");
}

void Slots(library_t *lib, int slots)
{
    if (slots < 0)
    {
        printf("IGNORED\n");
        return;
    }
    SLOTS = slots;
    printf("DONE\n");
}

/* compute points for a genre: sum of scores for non-lost, reviewed books */
static long long compute_genre_points(genre_t *g)
{
    long long s = 0;
    for (book_t *b = g->books; b; b = b->next)
    {
        if (b->lost_flag == 0 && b->n_reviews > 0)
            s += b->sum_scores;
    }
    return s;
}

void commandD(library_t *lib)
{
    DISPLAY_READY = 0;

    /* clear previous display */
    for (genre_t *g = lib->genres; g; g = g->next)
    {
        if (g->display)
        {
            free(g->display);
            g->display = NULL;
        }
        g->slots = 0;
    }

    if (SLOTS <= 0 || !lib->genres)
    {
        printf("DONE\n");
        return;
    }

    /* collect genres with points > 0 */
    int genre_count = 0;
    for (genre_t *g = lib->genres; g; g = g->next)
        genre_count++;

    if (genre_count == 0)
    {
        printf("DONE\n");
        return;
    }

    genre_t **array = malloc(sizeof(genre_t *) * genre_count);
    if (!array)
    {
        printf("DONE\n");
        return;
    }

    int idx = 0;
    long long totalPoints = 0;
    for (genre_t *g = lib->genres; g; g = g->next)
    {
        array[idx++] = g;
        long long pts = compute_genre_points(g);
        g->invalid_count = (int)pts; /* temporarily store points */
        totalPoints += pts;
    }

    if (totalPoints <= 0)
    {
        free(array);
        DISPLAY_READY = 1;
        printf("DONE\n");
        return;
    }

    int slotsAssigned = 0;

    for (int i = 0; i < genre_count; i++)
    {
        genre_t *g = array[i];
        long long pts = g->invalid_count;
        int baseSlots = (int)((pts * SLOTS) / totalPoints);
        g->slots = baseSlots;
        slotsAssigned += baseSlots;
    }

    /* naive leftover distribution: give remaining slots one by one in gid order */
    int remaining = SLOTS - slotsAssigned;
    int i = 0;
    while (remaining > 0 && genre_count > 0)
    {
        genre_t *g = array[i % genre_count];
        g->slots++;
        remaining--;
        i++;
    }

    /* allocate display arrays */
    for (int j = 0; j < genre_count; j++)
    {
        genre_t *g = array[j];
        if (g->slots > 0)
        {
            g->display = malloc(sizeof(book_t *) * g->slots);
            if (!g->display)
                continue;
            int k = 0;
            for (book_t *b = g->books; b && k < g->slots; b = b->next)
            {
                if (b->lost_flag == 0 && b->n_reviews > 0)
                    g->display[k++] = b;
            }
            while (k < g->slots)
            {
                g->display[k++] = NULL;
            }
        }
    }

    free(array);
    DISPLAY_READY = 1;
    printf("DONE\n");
}

void printPG(library_t *lib, int gid)
{
    genre_t *g = findeGenre(lib, gid);
    if (!g)
    {
        printf("IGNORED\n");
        return;
    }
    printf("\nGENRE WITH GID: %d\n", g->gid);
    printf("----------------Has Books---------------\n");
    for (book_t *b = g->books; b; b = b->next)
    {
        printf("<bid: %d>, <avg: %d>\n", b->bid, b->avg);
    }
}

void printPM(library_t *lib, int sid)
{
    member_t *m = findMember(&lib->members, sid);
    if (!m)
    {
        printf("IGNORED\n");
        return;
    }

    printf("\nMEMBER WITH SID: %d\n", m->sid);
    printf("Has Borrowed Books\n");
    if (!m->loans)
        return;

    int cap = 16, n = 0;
    int *arr = malloc(sizeof(int) * cap);
    if (!arr)
        return;

    for (loan_t *it = m->loans->next; it; it = it->next)
    {
        if (n >= cap)
        {
            cap *= 2;
            int *tmp = realloc(arr, sizeof(int) * cap);
            if (!tmp)
            {
                free(arr);
                return;
            }
            arr = tmp;
        }
        arr[n++] = it->bid;
    }
    for (int i = n - 1; i >= 0; i--)
    {
        printf("%d\n", arr[i]);
    }
    free(arr);
}

void printPD(library_t *lib)
{
    printf("Display:\n");

    if (!DISPLAY_READY)
    {
        printf("(empty)\n");
        return;
    }

    int any = 0;
    for (genre_t *g = lib->genres; g; g = g->next)
    {
        if (g->slots > 0 && g->display)
        {
            any = 1;
            break;
        }
    }
    if (!any)
    {
        printf("(empty)\n");
        return;
    }

    /* Print in ascending gid order */
    for (genre_t *g = lib->genres; g; g = g->next)
    {
        printf("%d:\n", g->gid);

        if (g->slots > 0 && g->display)
        {
            for (int i = 0; i < g->slots; i++)
            {
                book_t *b = g->display[i];
                if (b)
                    printf("<%d>, <%d>\n", b->bid, b->avg);
            }
        }
    }
}

void printPS(library_t *lib)
{
    printf("SLOTS=%d\n", SLOTS);
    for (genre_t *g = lib->genres; g; g = g->next)
    {
        long long pts = compute_genre_points(g);
        printf("%d: points=%lld\n", g->gid, pts);
    }
}

/* Debug printing */

void debugPrintAllBooks(const library_t *lib)
{
    if (!lib)
        return;

    printf("Books (by genre):\n");
    const genre_t *g = lib->genres;
    if (!g)
    {
        printf("(no genres)\n");
        return;
    }

    while (g)
    {
        debugPrintGenreBooks(g);
        g = g->next;
    }
}

void debugPrintGenreBooks(const genre_t *g)
{
    if (!g)
        return;

    printf("%d (%s):\n", g->gid, g->name);

    const book_t *b = g->books;
    if (!b)
    {
        printf("  (no books)\n");
        return;
    }

    while (b)
    {
        int prev_bid = b->prev ? b->prev->bid : -1;
        int next_bid = b->next ? b->next->bid : -1;

        printf("  bid=%d, avg=%d, reviews=%d, sum=%d, lost=%d, gid=%d, "
               "prev=%d, next=%d, title=\"%s\"\n",
               b->bid, b->avg, b->n_reviews, b->sum_scores, b->lost_flag,
               b->gid, prev_bid, next_bid, b->title);

        b = b->next;
    }
}

/* helper list  */

helper_t *create(long long data)
{
    helper_t *node = malloc(sizeof(helper_t));
    if (!node)
        return NULL;
    node->point = data;
    node->next = NULL;
    return node;
}

void insert(helper_t **headRef, long long data)
{
    helper_t *node = create(data);
    if (!node)
        return;
    node->next = *headRef;
    *headRef = node;
}

/*AVL (sorted by title)*/

int getHeight(bookIndex_t *node)
{
    return node ? node->height : 0;
}

int updateHeight(bookIndex_t *node)
{
    int hl = getHeight(node->lc);
    int hr = getHeight(node->rc);
    return (hl > hr ? hl : hr) + 1;
}

int getBalance(bookIndex_t *node)
{
    if (!node)
        return 0;
    return getHeight(node->lc) - getHeight(node->rc);
}

bookIndex_t *rotate_right(bookIndex_t *root)
{
    bookIndex_t *x = root->lc;
    bookIndex_t *T2 = x->rc;

    x->rc = root;
    root->lc = T2;

    root->height = updateHeight(root);
    x->height = updateHeight(x);

    return x;
}

bookIndex_t *rotate_left(bookIndex_t *root)
{
    bookIndex_t *y = root->rc;
    bookIndex_t *T2 = y->lc;

    y->lc = root;
    root->rc = T2;

    root->height = updateHeight(root);
    y->height = updateHeight(y);

    return y;
}

bookIndex_t *createBookIndexNode(book_t *book)
{
    bookIndex_t *n = malloc(sizeof(bookIndex_t));
    if (!n)
        return NULL;
    strncpy(n->title, book->title, TITLE_MAX);
    n->title[TITLE_MAX - 1] = '\0';
    n->book = book;
    n->lc = n->rc = NULL;
    n->height = 1;
    return n;
}

bookIndex_t *avl_insert(bookIndex_t *root, book_t *book)
{
    if (!root)
        return createBookIndexNode(book);

    int cmp = strcmp(book->title, root->title);
    if (cmp < 0)
        root->lc = avl_insert(root->lc, book);
    else if (cmp > 0)
        root->rc = avl_insert(root->rc, book);
    else
        return root; /* duplicate title not inserted again */

    root->height = updateHeight(root);
    int bf = getBalance(root);

    /* LL */
    if (bf > 1 && strcmp(book->title, root->lc->title) < 0)
        return rotate_right(root);
    /* RR */
    if (bf < -1 && strcmp(book->title, root->rc->title) > 0)
        return rotate_left(root);
    /* LR */
    if (bf > 1 && strcmp(book->title, root->lc->title) > 0)
    {
        root->lc = rotate_left(root->lc);
        return rotate_right(root);
    }
    /* RL */
    if (bf < -1 && strcmp(book->title, root->rc->title) < 0)
    {
        root->rc = rotate_right(root->rc);
        return rotate_left(root);
    }

    return root;
}

bookIndex_t *avl_search(bookIndex_t *root, const char *title)
{
    if (!root)
        return NULL;

    int cmp = strcmp(title, root->title);
    if (cmp == 0)
        return root;
    if (cmp < 0)
        return avl_search(root->lc, title);
    else
        return avl_search(root->rc, title);
}

/*Recomedation heap */
static int compareBooks(const book_t *a, const book_t *b)
{ /*bigger book the one with the bigger avg*/
    if (a->avg > b->avg)
        return 1;
    if (a->avg < b->avg)
        return 0;
    /*if avg equal then smaller bid bigger book */
    return (a->bid < b->bid);
}

RecHeap_t *recheapcrete(int capacity)
{
    RecHeap_t *heap = malloc(sizeof(RecHeap_t));
    if (!heap)
        return NULL;
    heap->heap = malloc(sizeof(book_t *) * (capacity + 1)); // we allocate the internal array of heap elements
    // we are creating  a block of memory that will store every book pointer to the heap (heap-> points to the array)
    if (!heap->heap)
    {
        free(heap);
        return NULL;
    }
    heap->size = 0;
    heap->capacity = capacity;
    return heap;
}

static void swapBooks(RecHeap_t *heap, int i, int j)
{
    book_t *temp = heap->heap[i];
    book_t *temp2 = heap->heap[j];
    heap->heap[i] = temp2;
    heap->heap[j] = temp;
    if (temp)
    {

        temp->heap_pos = j;
    }
    if (temp2)
    {

        temp2->heap_pos = i;
    }
}

static void HeapSiftUp(RecHeap_t *heap, int index)
{
    while (index > 1) // while not at root
    {
        int parent = index / 2;                                  // compute parent index
        if (compareBooks(heap->heap[index], heap->heap[parent])) // compare the avg of parent and child
                                                                 // if child is bigger than parent we swap
        {
            swapBooks(heap, index, parent);
            index = parent; // new position of element after swap
        }
        else
        {
            break;
        }
    }
}

static void HeapSiftDown(RecHeap_t *heap, int index)
{
    while (true)
    {
        int leftChild = index * 2;
        int rightChild = index * 2 + 1;
        int largest = index;
        // checking if left child exists and is bigger than current largest
        if (leftChild <= heap->size && compareBooks(heap->heap[leftChild], heap->heap[largest]))
        {
            largest = leftChild;
        }
        // checking if right child exists and is bigger than current largest
        if (rightChild <= heap->size && compareBooks(heap->heap[rightChild], heap->heap[largest]))
        {
            largest = rightChild;
        }
        if (largest != index)
        {
            swapBooks(heap, index, largest);
            index = largest;
        }
        else
        {
            break;
        }
    }
}
static void HeapRemoveIndex(RecHeap_t *heap, int index)
{
    if (index < 1 || index > heap->size)
        return; // Invalid index
    book_t *removedBook = heap->heap[index];
    if (removedBook)
        removedBook->heap_pos = -1; // Mark as not in heap
    if (index == heap->size)
    {
        heap->size--;
        return;
    }
    heap->heap[index] = heap->heap[heap->size];
    if (heap->heap[index])
        heap->heap[index]->heap_pos = index;
    heap->size--;
    HeapSiftDown(heap, index);
    HeapSiftUp(heap, index);
}
static int WorstBookIndex(RecHeap_t *heap)
{
    if (heap->size == 0)
        return -1;
    int worstIndex = 1; // assuming root worst
    for (int i = 2; i <= heap->size; i++)
    {
        if (!compareBooks(heap->heap[i], heap->heap[worstIndex]))
        {
            worstIndex = i;
        }
    }
    return worstIndex;
}

// called when a book's avg is updated, or a lost_flag changes
// decides whether to insert, remove or update the book in the recommendation heap
static int findMinLeafIndex(RecHeap_t *heap)
{
    if (heap->size == 0)
        return -1;
    int min_idx = (heap->size / 2) + 1;
    if (min_idx > heap->size)
        min_idx = 1;
    for (int i = min_idx + 1; i <= heap->size; i++)
    {
        /* compareBooks(a, b) returns 1 if a > b. We want min. */
        if (compareBooks(heap->heap[min_idx], heap->heap[i]))
        {
            min_idx = i;
        }
    }
    return min_idx;
}
void UpdateRecommendationHeap(RecHeap_t *heap, book_t *book)
{
    if (!heap || !book)
        return;
    // book is lost or no valid rewies, must not be in hte heap
    if (book->lost_flag || book->n_reviews == 0)
    {
        if (book->heap_pos != -1)
            HeapRemoveIndex(heap, book->heap_pos);
        return;
    }
    // book should be in the heap
    if (book->heap_pos == -1)
    {
        // not in heap, insert if space or better than worst
        if (heap->size < heap->capacity)
        {
            heap->size++;
            int index = heap->size;
            heap->heap[index] = book;
            book->heap_pos = index;
            HeapSiftUp(heap, index);
        }
        else
        { // heap full check which book is better
            int worstIndex = findMinLeafIndex(heap);
            if (worstIndex != -1)
            {
                book_t *worstBook = heap->heap[worstIndex];

                if (compareBooks(book, worstBook))
                {
                    // remove worst and insert new book
                    worstBook->heap_pos = -1;
                    heap->heap[worstIndex] = book;
                    book->heap_pos = worstIndex;
                    HeapSiftUp(heap, worstIndex);

                } // else {
                  // do nothing, book not good enough for heap
            }
        }
        return;
    }

    // already in heap, just sift up or down
    HeapSiftUp(heap, book->heap_pos);
    HeapSiftDown(heap, book->heap_pos);
}

void commandTOP(library_t *lib, int k)
{
    RecHeap_t *h = lib->recommendations;

    printf("Top Books:\n");

    if (!h || h->size == 0 || k <= 0)
        return;

    if (k > h->size)
        k = h->size;

    /* 1. Δέσμευση πίνακα για αντιγραφή των δεικτών */
    book_t **tempArray = malloc(sizeof(book_t *) * h->size);
    if (!tempArray)
        return;

    /* 2. Αντιγραφή όλων των βιβλίων από το heap στον πίνακα */
    for (int i = 1; i <= h->size; i++)
    {
        tempArray[i - 1] = h->heap[i];
    }

    /* 3. Ταξινόμηση του πίνακα (Bubble Sort) - ΑΣΦΑΛΕΣ */
    for (int i = 0; i < h->size - 1; i++)
    {
        for (int j = 0; j < h->size - i - 1; j++)
        {
            /* compareBooks: επιστρέφει 1 αν a > b. Θέλουμε φθίνουσα, άρα swap αν b > a */
            if (compareBooks(tempArray[j + 1], tempArray[j]))
            {
                book_t *temp = tempArray[j];
                tempArray[j] = tempArray[j + 1];
                tempArray[j + 1] = temp;
            }
        }
    }

    /* 4. Εκτύπωση των k πρώτων */
    for (int i = 0; i < k; i++)
    {
        book_t *best = tempArray[i];
        printf("%d \"%s\" avg=%d\n", best->bid, best->title, best->avg);
    }

    free(tempArray);
}

MemberActivity_t *createMemberActivity(library_t *lib, int sid)
{

    member_t *m = findMember(&lib->members, sid);
    if (m && m->activity)
    {
        return m->activity;
    }
    MemberActivity_t *current = lib->activity;
    while (current)
    {
        if (current->sid == sid)
        {
            if (m)
            {
                m->activity = current;
            }
            return current;
        }
        current = current->next;
    }
    // create new member activity
    MemberActivity_t *act = malloc(sizeof(MemberActivity_t));
    act->sid = sid;
    act->loans_count = 0;
    act->reviews_count = 0;
    act->score_sum = 0;
    act->next = lib->activity;
    act->prev = NULL;

    // Insert into the list
    if (lib->activity)
    {
        lib->activity->prev = act;
    }

    lib->activity = act;
    if (m)
    {
        m->activity = act;
    }
    return act;
}

bookIndex_t *avl_find_min(bookIndex_t *root)
{
    while (root->lc)
        root = root->lc;
    return root;
}

bookIndex_t *avl_delete(bookIndex_t *root, const char *title)
{
    if (!root)
        return NULL;

    int cmp = strcmp(title, root->title);

    // searching for the node to delete
    if (cmp < 0)
    {
        root->lc = avl_delete(root->lc, title);
    }
    else if (cmp > 0)
    {
        root->rc = avl_delete(root->rc, title);
    }
    else
    {

        /* Case A: no children */
        if (!root->lc && !root->rc)
        {
            free(root);
            return NULL;
        }

        /* Case B: one child */
        else if (!root->lc || !root->rc)
        {
            bookIndex_t *child = (root->lc) ? root->lc : root->rc;
            free(root);
            return child;
        }

        /* Case C: two children
           Replace with inorder successor (min of right subtree)
        */
        bookIndex_t *minNode = avl_find_min(root->rc);

        /* Copy successor's data into this node */
        strcpy(root->title, minNode->title);
        root->book = minNode->book;

        /* Delete successor recursively */
        root->rc = avl_delete(root->rc, minNode->title);
    }

    root->height = updateHeight(root);

    int balance = getBalance(root);

    /* LL CASE */
    if (balance > 1 && getBalance(root->lc) >= 0)
        return rotate_right(root);

    /* LR CASE */
    if (balance > 1 && getBalance(root->lc) < 0)
    {
        root->lc = rotate_left(root->lc);
        return rotate_right(root);
    }

    /* RR CASE */
    if (balance < -1 && getBalance(root->rc) <= 0)
        return rotate_left(root);

    /* RL CASE */
    if (balance < -1 && getBalance(root->rc) > 0)
    {
        root->rc = rotate_right(root->rc);
        return rotate_left(root);
    }

    /* Tree already balanced */
    return root;
}
int is_better_activity(MemberActivity_t *a, MemberActivity_t *b)
{
    int scoreA = a->loans_count + a->reviews_count;
    int scoreB = b->loans_count + b->reviews_count;
    if (scoreA > scoreB)
        return 1;
    if (scoreA < scoreB)
        return 0;
    return (a->sid < b->sid);
}
void repositionActivity(library_t *lib, MemberActivity_t *act)
{
    if (!act)
        return;

    while (act->prev && is_better_activity(act, act->prev))
    {
        MemberActivity_t *p = act->prev;
        MemberActivity_t *n = act->next;
        MemberActivity_t *pp = p->prev;

        /* connect pp  act */
        if (pp)
            pp->next = act;
        else
            lib->activity = act;

        act->prev = pp;
        act->next = p;

        /* connect p below act */
        p->prev = act;
        p->next = n;

        if (n)
            n->prev = p;
    }
}
void commandAM(library_t *lib)
{
    printf("Active Members:\n");
   
    for (MemberActivity_t *p = lib->activity; p; p = p->next)
    {
        member_t *m = findMember(&lib->members, p->sid);
        printf("%d %s loans=%d reviews=%d\n",
               p->sid, m ? m->name : "", p->loans_count, p->reviews_count);
    }
}

void commandU(library_t *lib, int bid, const char *new_title)
{

    book_t *b = findBook(lib, bid);

    if (!b)
    {
        printf("IGNORED\n");
        return;
    }

    genre_t *g = findeGenre(lib, b->gid);

    if (avl_search(g->bookIndex, new_title) != NULL)
    {
        printf("IGNORED\n");
        return;
    } // checking for duplicate titles

    /* FIX: Update in Genre's AVL */

    g->bookIndex = avl_delete(g->bookIndex, b->title);

    strncpy(b->title, new_title, TITLE_MAX);
    b->title[TITLE_MAX - 1] = '\0';

    g->bookIndex = avl_insert(g->bookIndex, b);

    printf("DONE\n");
}
void commandX(library_t *lib)
{
    double avg = 0.0;
    if (lib->total_reviews_count > 0)
        avg = (double)lib->total_review_score / lib->total_reviews_count;

    printf("Stats:\n");
    printf("books=%d\n", lib->num_books);
    printf("members=%d\n", lib->num_members); //  lib->member_count
    printf("loans=%d\n", lib->num_active_loans);
    printf("avg_reviews=%d\n", (int)avg); 
}

/* Free all allocated memory */
void freeBooks(book_t *b)
{
    while (b)
    {
        book_t *next = b->next;
        free(b);
        b = next;
    }
}
void freeGenres(genre_t *g)
{
    while (g)
    {
        genre_t *next = g->next;
        freeBooks(g->books);
        if (g->display)
            free(g->display);
        free(g);
        g = next;
    }
}
void freeLoans(loan_t *loans)
{
    while (loans)
    {
        loan_t *next = loans->next;
        free(loans);
        loans = next;
    }
}
void freeMembers(member_t *members)
{
    while (members)
    {
        member_t *next = members->next;
        freeLoans(members->loans);
        free(members);
        members = next;
    }
}

void freeAVL(bookIndex_t *root)
{
    if (!root)
        return;
    freeAVL(root->lc);
    freeAVL(root->rc);
    free(root);
}

void freeRecommendationHeap(RecHeap_t *heap)
{
    if (!heap)
        return;
    if (heap->heap)
        free(heap->heap);

    free(heap);
}
void freeMemberActivities(MemberActivity_t *activity)
{
    while (activity)
    {
        MemberActivity_t *next = activity->next;
        free(activity);
        activity = next;
    }
}
void commandBF(library_t *lib)
{
    /* 1. Free Members & Loans FIRST  */
    while (lib->members)
    {
        member_t *nm = lib->members->next;

        
        while (lib->members->loans)
        {
            loan_t *curr_loan = lib->members->loans;
            loan_t *next_loan = curr_loan->next;

            

            if (curr_loan->bid > 0)
            { 
                book_t *b = findBook(lib, curr_loan->bid);
                if (b)
                {
                    b->is_loaned = 0; /* Reset flag */
                }
                lib->num_active_loans--; /* Update Stats */
            }

            /* DELETE & FREE */
            free(curr_loan);

            lib->members->loans = next_loan;
        }

        free(lib->members);
        lib->members = nm;
    }
    lib->member_count = 0;
    lib->num_members = 0;      /* Reset Stat */
    lib->num_active_loans = 0; /* Reset Stat  */

    /* 2. Free Genres & Books & AVLs  */
    while (lib->genres)
    {
        genre_t *next = lib->genres->next;

        /* Free AVL */
        freeAVL(lib->genres->bookIndex);

        /* Free Books List */
        while (lib->genres->books)
        {
            book_t *nb = lib->genres->books->next;
            free(lib->genres->books);
            lib->genres->books = nb;
        }

        if (lib->genres->display)
            free(lib->genres->display);

        free(lib->genres);
        lib->genres = next;
    }
    lib->genre_count = 0;
    lib->num_books = 0; /* Reset Stat */

    /* 3. Free Heap & Activity */
    freeRecommendationHeap(lib->recommendations);
    lib->recommendations = recheapcrete(REC_HEAP_CAPACITY);

    freeMemberActivities(lib->activity);
    lib->activity = NULL;

    /* Reset Ï…Ï€ÏŒÎ»Î¿Î¹Ï€Î± Stats */
    lib->total_review_score = 0;
    lib->total_reviews_count = 0;

    DISPLAY_READY = 0;
    printf("DONE\n");
}