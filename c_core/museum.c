#include "museum.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int exhibitID;
    double revenue;
    int tickets;
} ExhibitRevenue;

static void trim_newline(char *text)
{
    text[strcspn(text, "\n")] = '\0';
}

static int read_int(const char *prompt, int *value)
{
    char input[64];
    char *end;
    long number;

    printf("%s", prompt);

    if (!fgets(input, sizeof(input), stdin)) {
        return 0;
    }

    number = strtol(input, &end, 10);

    if (end == input) {
        return 0;
    }

    while (*end == ' ' || *end == '\t') {
        end++;
    }

    if (*end != '\n' && *end != '\0') {
        return 0;
    }

    *value = (int)number;

    return 1;
}

static int read_double(const char *prompt, double *value)
{
    char input[64];
    char *end;
    double number;

    printf("%s", prompt);

    if (!fgets(input, sizeof(input), stdin)) {
        return 0;
    }

    number = strtod(input, &end);

    if (end == input) {
        return 0;
    }

    while (*end == ' ' || *end == '\t') {
        end++;
    }

    if (*end != '\n' && *end != '\0') {
        return 0;
    }

    *value = number;

    return 1;
}

static int next_exhibit_id(void)
{
    FILE *file = fopen(EXHIBIT_FILE, "rb");

    if (!file) {
        return 1;
    }

    Exhibit exhibit;
    int highest_id = 0;

    while (fread(&exhibit, sizeof(Exhibit), 1, file) == 1) {
        if (exhibit.exhibitID > highest_id) {
            highest_id = exhibit.exhibitID;
        }
    }

    fclose(file);

    return highest_id + 1;
}

static int next_ticket_id(void)
{
    FILE *file = fopen(TICKET_FILE, "rb");

    if (!file) {
        return 1;
    }

    Ticket ticket;
    int highest_id = 0;

    while (fread(&ticket, sizeof(Ticket), 1, file) == 1) {
        if (ticket.ticketID > highest_id) {
            highest_id = ticket.ticketID;
        }
    }

    fclose(file);

    return highest_id + 1;
}

int add_exhibit(const char *name, const char *category)
{
    if (!name || !category ||
        strlen(name) == 0 ||
        strlen(category) == 0) {
        return 0;
    }

    FILE *file = fopen(EXHIBIT_FILE, "ab");

    if (!file) {
        perror("Unable to open exhibit file");
        return 0;
    }

    Exhibit exhibit;

    exhibit.exhibitID = next_exhibit_id();
    exhibit.visitorCount = 0;

    snprintf(exhibit.name, NAME_MAX, "%s", name);
    snprintf(exhibit.category, CATEGORY_MAX, "%s", category);

    if (fwrite(&exhibit, sizeof(Exhibit), 1, file) != 1) {
        perror("Unable to save exhibit");
        fclose(file);
        return 0;
    }

    fclose(file);

    printf("Exhibit added successfully. ID: %d\n",
           exhibit.exhibitID);

    return exhibit.exhibitID;
}

int search_exhibit_by_id(int id, Exhibit *result)
{
    FILE *file = fopen(EXHIBIT_FILE, "rb");

    if (!file) {
        return 0;
    }

    Exhibit exhibit;

    while (fread(&exhibit, sizeof(Exhibit), 1, file) == 1) {
        if (exhibit.exhibitID == id) {
            if (result) {
                *result = exhibit;
            }

            fclose(file);
            return 1;
        }
    }

    fclose(file);

    return 0;
}

int list_exhibits(void)
{
    FILE *file = fopen(EXHIBIT_FILE, "rb");

    if (!file) {
        printf("No exhibits found.\n");
        return 0;
    }

    Exhibit exhibit;
    int count = 0;

    printf("\n%-5s %-30s %-20s %-10s\n",
           "ID",
           "Exhibit",
           "Category",
           "Visitors");

    printf("------------------------------------------------------------------\n");

    while (fread(&exhibit, sizeof(Exhibit), 1, file) == 1) {
        printf("%-5d %-30s %-20s %-10d\n",
               exhibit.exhibitID,
               exhibit.name,
               exhibit.category,
               exhibit.visitorCount);

        count++;
    }

    fclose(file);

    if (count == 0) {
        printf("No exhibits found.\n");
    }

    return count;
}

int delete_exhibit(int id)
{
    FILE *source = fopen(EXHIBIT_FILE, "rb");

    if (!source) {
        return 0;
    }

    FILE *temporary = fopen("data/exhibits.tmp", "wb");

    if (!temporary) {
        fclose(source);
        return 0;
    }

    Exhibit exhibit;
    int found = 0;

    while (fread(&exhibit, sizeof(Exhibit), 1, source) == 1) {
        if (exhibit.exhibitID == id) {
            found = 1;
            continue;
        }

        if (fwrite(&exhibit, sizeof(Exhibit), 1, temporary) != 1) {
            fclose(source);
            fclose(temporary);
            remove("data/exhibits.tmp");
            return 0;
        }
    }

    fclose(source);
    fclose(temporary);

    if (!found) {
        remove("data/exhibits.tmp");
        return 0;
    }

    if (remove(EXHIBIT_FILE) != 0) {
        remove("data/exhibits.tmp");
        return 0;
    }

    if (rename("data/exhibits.tmp", EXHIBIT_FILE) != 0) {
        return 0;
    }

    return 1;
}

int purchase_ticket(int exhibit_id,
                    const char *visitor_name,
                    double price)
{
    if (!visitor_name ||
        strlen(visitor_name) == 0 ||
        price <= 0) {
        return 0;
    }

    Exhibit exhibit;

    if (!search_exhibit_by_id(exhibit_id, &exhibit)) {
        return 0;
    }

    FILE *ticket_file = fopen(TICKET_FILE, "ab");

    if (!ticket_file) {
        perror("Unable to open ticket file");
        return 0;
    }

    Ticket ticket;

    ticket.ticketID = next_ticket_id();
    ticket.exhibitID = exhibit_id;
    ticket.ticketPrice = price;

    snprintf(ticket.visitorName,
             NAME_MAX,
             "%s",
             visitor_name);

    if (fwrite(&ticket, sizeof(Ticket), 1, ticket_file) != 1) {
        fclose(ticket_file);
        return 0;
    }

    fclose(ticket_file);

    FILE *exhibit_file = fopen(EXHIBIT_FILE, "r+b");

    if (!exhibit_file) {
        return 0;
    }

    while (fread(&exhibit, sizeof(Exhibit), 1, exhibit_file) == 1) {
        if (exhibit.exhibitID == exhibit_id) {
            exhibit.visitorCount++;

            fseek(exhibit_file,
                  -(long)sizeof(Exhibit),
                  SEEK_CUR);

            fwrite(&exhibit,
                   sizeof(Exhibit),
                   1,
                   exhibit_file);

            break;
        }
    }

    fclose(exhibit_file);

    printf("Ticket purchased successfully. Ticket ID: %d\n",
           ticket.ticketID);

    return ticket.ticketID;
}

int list_tickets(void)
{
    FILE *file = fopen(TICKET_FILE, "rb");

    if (!file) {
        printf("No tickets found.\n");
        return 0;
    }

    Ticket ticket;
    int count = 0;

    printf("\n%-8s %-10s %-25s %-10s\n",
           "Ticket",
           "Exhibit",
           "Visitor",
           "Price");

    printf("---------------------------------------------------------------\n");

    while (fread(&ticket, sizeof(Ticket), 1, file) == 1) {
        printf("%-8d %-10d %-25s $%-9.2f\n",
               ticket.ticketID,
               ticket.exhibitID,
               ticket.visitorName,
               ticket.ticketPrice);

        count++;
    }

    fclose(file);

    return count;
}

int report_revenue(void)
{
    FILE *file = fopen(TICKET_FILE, "rb");

    if (!file) {
        printf("No ticket data available.\n");
        return 0;
    }

    ExhibitRevenue *records = NULL;
    size_t count = 0;

    Ticket ticket;

    while (fread(&ticket, sizeof(Ticket), 1, file) == 1) {
        size_t index;

        for (index = 0; index < count; index++) {
            if (records[index].exhibitID == ticket.exhibitID) {
                records[index].revenue += ticket.ticketPrice;
                records[index].tickets++;
                break;
            }
        }

        if (index == count) {
            ExhibitRevenue *expanded =
                realloc(records,
                        (count + 1) * sizeof(ExhibitRevenue));

            if (!expanded) {
                free(records);
                fclose(file);
                return 0;
            }

            records = expanded;

            records[count].exhibitID = ticket.exhibitID;
            records[count].revenue = ticket.ticketPrice;
            records[count].tickets = 1;

            count++;
        }
    }

    fclose(file);

    printf("\nRevenue by Exhibit\n");
    printf("---------------------------------------------\n");

    printf("%-10s %-15s %-15s\n",
           "Exhibit",
           "Tickets",
           "Revenue");

    for (size_t i = 0; i < count; i++) {
        printf("%-10d %-15d $%-14.2f\n",
               records[i].exhibitID,
               records[i].tickets,
               records[i].revenue);
    }

    free(records);

    return (int)count;
}

static int compare_exhibits(const void *a, const void *b)
{
    const Exhibit *first = a;
    const Exhibit *second = b;

    if (first->visitorCount < second->visitorCount) {
        return 1;
    }

    if (first->visitorCount > second->visitorCount) {
        return -1;
    }

    return 0;
}

int report_popular_exhibits(void)
{
    FILE *file = fopen(EXHIBIT_FILE, "rb");

    if (!file) {
        printf("No exhibit data available.\n");
        return 0;
    }

    Exhibit *exhibits = NULL;
    size_t count = 0;

    Exhibit exhibit;

    while (fread(&exhibit, sizeof(Exhibit), 1, file) == 1) {
        Exhibit *expanded =
            realloc(exhibits,
                    (count + 1) * sizeof(Exhibit));

        if (!expanded) {
            free(exhibits);
            fclose(file);
            return 0;
        }

        exhibits = expanded;
        exhibits[count] = exhibit;
        count++;
    }

    fclose(file);

    qsort(exhibits,
          count,
          sizeof(Exhibit),
          compare_exhibits);

    printf("\nPopular Exhibits\n");
    printf("---------------------------------------------\n");

    printf("%-6s %-30s %-10s\n",
           "Rank",
           "Exhibit",
           "Visitors");

    for (size_t i = 0; i < count; i++) {
        printf("%-6zu %-30s %-10d\n",
               i + 1,
               exhibits[i].name,
               exhibits[i].visitorCount);
    }

    free(exhibits);

    return (int)count;
}

int get_dashboard(int *exhibits,
                  int *visitors,
                  int *tickets,
                  double *revenue)
{
    if (!exhibits ||
        !visitors ||
        !tickets ||
        !revenue) {
        return 0;
    }

    *exhibits = 0;
    *visitors = 0;
    *tickets = 0;
    *revenue = 0.0;

    FILE *exhibit_file = fopen(EXHIBIT_FILE, "rb");

    if (exhibit_file) {
        Exhibit exhibit;

        while (fread(&exhibit,
                     sizeof(Exhibit),
                     1,
                     exhibit_file) == 1) {

            (*exhibits)++;
            *visitors += exhibit.visitorCount;
        }

        fclose(exhibit_file);
    }

    FILE *ticket_file = fopen(TICKET_FILE, "rb");

    if (ticket_file) {
        Ticket ticket;

        while (fread(&ticket,
                     sizeof(Ticket),
                     1,
                     ticket_file) == 1) {

            (*tickets)++;
            *revenue += ticket.ticketPrice;
        }

        fclose(ticket_file);
    }

    return 1;
}

static void interactive_add_exhibit(void)
{
    char name[NAME_MAX];
    char category[CATEGORY_MAX];

    printf("\nExhibit name: ");

    if (!fgets(name, sizeof(name), stdin)) {
        return;
    }

    trim_newline(name);

    if (strlen(name) == 0) {
        printf("Exhibit name cannot be empty.\n");
        return;
    }

    printf("Category: ");

    if (!fgets(category, sizeof(category), stdin)) {
        return;
    }

    trim_newline(category);

    if (strlen(category) == 0) {
        printf("Category cannot be empty.\n");
        return;
    }

    add_exhibit(name, category);
}

static void interactive_purchase(void)
{
    int exhibit_id;
    double price;
    char visitor[NAME_MAX];

    if (!read_int("\nExhibit ID: ", &exhibit_id)) {
        printf("Invalid exhibit ID.\n");
        return;
    }

    printf("Visitor name: ");

    if (!fgets(visitor, sizeof(visitor), stdin)) {
        return;
    }

    trim_newline(visitor);

    if (strlen(visitor) == 0) {
        printf("Visitor name cannot be empty.\n");
        return;
    }

    if (!read_double("Ticket price: ", &price)) {
        printf("Invalid price.\n");
        return;
    }

    if (!purchase_ticket(exhibit_id,
                         visitor,
                         price)) {

        printf("Unable to purchase ticket. ");
        printf("Check the exhibit and price.\n");
    }
}

void run_menu(void)
{
    int choice;

    do {
        printf("\n========================================\n");
        printf("       MUSEUM MANAGEMENT SYSTEM\n");
        printf("========================================\n");
        printf("1. Add Exhibit\n");
        printf("2. List Exhibits\n");
        printf("3. Search Exhibit\n");
        printf("4. Purchase Ticket\n");
        printf("5. List Tickets\n");
        printf("6. Revenue Report\n");
        printf("7. Popular Exhibits\n");
        printf("8. Dashboard\n");
        printf("9. Delete Exhibit\n");
        printf("0. Exit\n");
        printf("----------------------------------------\n");

        if (!read_int("Choose an option: ", &choice)) {
            printf("Please enter a number from 0 to 9.\n");
            continue;
        }

        switch (choice) {

            case 1:
                interactive_add_exhibit();
                break;

            case 2:
                list_exhibits();
                break;

            case 3: {
                int id;
                Exhibit exhibit;

                if (!read_int("Exhibit ID: ", &id)) {
                    printf("Invalid exhibit ID.\n");
                    break;
                }

                if (search_exhibit_by_id(id, &exhibit)) {

                    printf("\nID: %d\n",
                           exhibit.exhibitID);

                    printf("Name: %s\n",
                           exhibit.name);

                    printf("Category: %s\n",
                           exhibit.category);

                    printf("Visitors: %d\n",
                           exhibit.visitorCount);

                } else {
                    printf("Exhibit not found.\n");
                }

                break;
            }

            case 4:
                interactive_purchase();
                break;

            case 5:
                list_tickets();
                break;

            case 6:
                report_revenue();
                break;

            case 7:
                report_popular_exhibits();
                break;

            case 8: {
                int exhibits;
                int visitors;
                int tickets;
                double revenue;

                get_dashboard(
                    &exhibits,
                    &visitors,
                    &tickets,
                    &revenue
                );

                printf("\nDashboard\n");
                printf("--------------------------------\n");
                printf("Exhibits : %d\n", exhibits);
                printf("Visitors : %d\n", visitors);
                printf("Tickets  : %d\n", tickets);
                printf("Revenue  : $%.2f\n", revenue);

                break;
            }

            case 9: {
                int id;

                if (!read_int(
                        "Exhibit ID to delete: ",
                        &id)) {

                    printf("Invalid exhibit ID.\n");
                    break;
                }

                if (delete_exhibit(id)) {
                    printf("Exhibit deleted successfully.\n");
                } else {
                    printf("Exhibit could not be deleted.\n");
                }

                break;
            }

            case 0:
                printf("Goodbye.\n");
                break;

            default:
                printf("Invalid option. Choose 0-9.\n");
                break;
        }

    } while (choice != 0);
}