#ifndef MUSEUM_H
#define MUSEUM_H

#include <stddef.h>

#define NAME_MAX 100
#define CATEGORY_MAX 60

#define EXHIBIT_FILE "data/exhibits.dat"
#define TICKET_FILE "data/tickets.dat"

typedef struct {
    int exhibitID;
    char name[NAME_MAX];
    char category[CATEGORY_MAX];
    int visitorCount;
} Exhibit;

typedef struct {
    int ticketID;
    int exhibitID;
    char visitorName[NAME_MAX];
    double ticketPrice;
} Ticket;

/* Exhibit operations */
int add_exhibit(const char *name, const char *category);
int list_exhibits(void);
int search_exhibit_by_id(int id, Exhibit *result);
int delete_exhibit(int id);

/* Ticket operations */
int purchase_ticket(int exhibit_id, const char *visitor_name, double price);
int list_tickets(void);

/* Reports */
int report_revenue(void);
int report_popular_exhibits(void);

/* Dashboard data */
int get_dashboard(int *exhibits, int *visitors, int *tickets, double *revenue);

/* Interactive application */
void run_menu(void);

#endif