#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include "headers.h"

void displayMenu(){
    printf("Menu to choose from\n");
    printf("a. To exit the menu\n");
    printf("b. To see all the products available\n");
    printf("c. To see your cart\n");
    printf("d. To add products to your cart\n");
    printf("e. To edit an existing product in your cart\n");
    printf("f. To proceed for payment\n");
    printf("Please enter your choice\n");
}

void printProduct(struct product p){
    printf("%d\t%s\t%d\t%d\n", p.id, p.name, p.qty, p.price);
}

void generateReceipt(int total, struct cart c){
    printf("ProductID\tProductName\tQuantityInStock\tPrice\n");
    for (int i=0; i<MAX_PROD; i++){
        printProduct(c.products[i]);
    }
    printf("Total %d\n", total);
}

int main(){
    printf("Establishing connection to server\n");
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1){
        perror("Error: ");
        return -1;
    }

    struct sockaddr_in serv;
    
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(5555);

    if (connect(sockfd, (struct sockaddr *)&serv, sizeof(serv)) == -1){
        perror("Error: ");
        return -1;
    }

    printf("Success\n");

    displayMenu();
    char ch;

    while (1){
        scanf("%d",&ch);

        write(sockfd, &ch, sizeof(char));

        if (ch == 'a'){
            break;
        }
        else if (ch == 'b'){
            printf("Fetching data\n");
            printf("ProductID\tProductName\tQuantityInStock\tPrice\n");
            while (1){
                struct product p;
                read(sockfd, &p, sizeof(struct product));
                if (p.id != -1){
                    printProduct(p);
                }else{
                    break;
                }
            }
        }
        else if (ch == 'c'){
            int cusid;
            printf("Enter customer id\n");
            scanf("%d", &cusid);
            write(sockfd, &cusid, sizeof(int));

            // while (1){
            struct cart o;
            read(sockfd, &o, sizeof(struct cart));

            if (o.custid != -1){
                printf("Customer ID %d", o.custid);
                printf("ProductID\tProductName\tQuantityInStock\tPrice\n");
                for (int i=0; i<MAX_PROD; i++){
                    printProduct(o.products[i]);
                }
            }else{
                printf("Wrong customer id provided\n");
            }
        }

        else if (ch == 'd'){
            int cusid;
            printf("Enter customer id\n");
            scanf("%d", &cusid);
            write(sockfd, &cusid, sizeof(int));

            int noprod;
            printf("Enter number of products\n");
            scanf("%d", noprod);

            for (int i=0; i<noprod; i++){
                int pid, qty;
                printf("Enter productId to order\n");
                scanf("%d\n", &pid);
                printf("Enter quantity");
                scanf("%d\n", &qty);

                struct product p;
                p.id = pid;
                p.qty = qty;

                write(sockfd, &p, sizeof(struct product));

                char response[20];
                read(sockfd, response, sizeof(response));
                printf("%s", response);
            }
        }

        else if (ch == 'e'){
            int cusid;
            printf("Enter customer id\n");
            scanf("%d", &cusid);
            write(sockfd, &cusid, sizeof(int));

            int pid, qty;
            printf("Enter productId to modify\n");
            scanf("%d\n", &pid);
            printf("Enter new quantity");
            scanf("%d\n", &qty);

            struct product p;
            p.id = pid;
            p.qty = qty;

            write(sockfd, &p, sizeof(struct product));

            char response[20];
            read(sockfd, response, sizeof(response));
            printf("%s", response);
        }

        else if (ch == 'f'){
            int cusid;
            printf("Enter customer id\n");
            scanf("%d", &cusid);
            write(sockfd, &cusid, sizeof(int));

            int total;
            
            printf("Total in your cart\n");
            read(sockfd, &total, sizeof(int));
            printf("%d\n", total);


            while (1){
                printf("Please enter the amount to pay\n");
                int payment;
                scanf("%d", &payment);

                if (payment != total){
                    printf("Wrong total entered, enter again\n");
                }else{
                    break;
                }
            }
            printf("Payment recorded, order placed");
            struct cart c;
            read(sockfd, &c, sizeof(struct cart));
            generateReceipt(total, c);
        }else{
            printf("Invalid choice, try again\n");
        }

        displayMenu();
    }

    printf("Exiting program\n");
    close(sockfd);
    return 0;
}