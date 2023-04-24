#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "headers.h"

void displayMenuUser(){
    printf("Menu to choose from\n");
    printf("a. To exit the menu\n");
    printf("b. To see all the products available\n");
    printf("c. To see your cart\n");
    printf("d. To add products to your cart\n");
    printf("e. To edit an existing product in your cart\n");
    printf("f. To proceed for payment\n");
    printf("g. To register a new customer\n");
    printf("Please enter your choice\n");
}

void displayMenuAdmin(){
    printf("Menu to choose from\n");
    printf("a. To add a product\n");
    printf("b. To delete a product\n");
    printf("c. To update the price of an existing product\n");
    printf("d. To update the quantity of an existing product\n");
    printf("e. To see your inventory\n");
    printf("f. To exit the program\n");
    printf("Please enter your choice\n");
}


void printProduct(struct product p){
    if (p.id != -1 && p.qty > 0){
        printf("%d\t%s\t%d\t%d\n", p.id, p.name, p.qty, p.price);
    }
}

int calculateTotal(struct cart c){
    int total = 0;
    for (int i=0; i<MAX_PROD; i++){
        total += c.products[i].qty * c.products[i].price;
    }

    return total;

}

void generateReceipt(int total, struct cart c){
    printf("ProductID\tProductName\tQuantity\tPrice\n");
    for (int i=0; i<MAX_PROD; i++){
        printProduct(c.products[i]);
    }
    printf("Total - %d\n", total);
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
    printf("Are you a user or the admin? Press 1 for user, 2 for admin\n");
    int user;
    scanf("%d", &user);
    write(sockfd, &user, sizeof(user));

    if (user == 1){
        displayMenuUser();
        char ch;
        scanf("%c",&ch);
        while (1){
            scanf("%c",&ch);

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

                int res;
                read(sockfd, &res, sizeof(int));
                if (res == -1){
                    printf("Invalid customer id\n");
                    continue;
                }

                int noprod;
                printf("Enter number of products\n");
                scanf("%d", &noprod);
                write(sockfd, &noprod, sizeof(int));

                char response[80];

                for (int i=0; i<noprod; i++){
                    int pid, qty;
                    printf("Enter productId to order\n");
                    scanf("%d\n", &pid);
                    
                    while (1){
                        printf("Enter new quantity\n");
                        scanf("%d\n", &qty);

                        if (qty <= 0){
                            printf("Quantity can't be <= 0, try again\n");
                        }else{
                            break;
                        }
                    }

                    struct product p;
                    p.id = pid;
                    p.qty = qty;

                    write(sockfd, &p, sizeof(struct product));
                    read(sockfd, response, sizeof(response));
                    printf("%s", response);
                }
            }

            else if (ch == 'e'){
                int cusid;
                printf("Enter customer id\n");
                scanf("%d", &cusid);
                write(sockfd, &cusid, sizeof(int));

                int res;
                read(sockfd, &res, sizeof(int));
                if (res == -1){
                    printf("Invalid customer id\n");
                    continue;
                }

                int pid, qty;
                printf("Enter productId to modify\n");
                scanf("%d\n", &pid);

                while (1){
                    printf("Enter new quantity, enter 0 to remove product from order\n");
                    scanf("%d\n", &qty);

                    if (qty < 0){
                        printf("Quantity can't be < 0, try again\n");
                    }else{
                        break;
                    }
                }
                
                struct product p;
                p.id = pid;
                p.qty = qty;

                write(sockfd, &p, sizeof(struct product));

                char response[80];
                read(sockfd, response, sizeof(response));
                printf("%s", response);
            }

            else if (ch == 'f'){
                int cusid;
                printf("Enter customer id\n");
                scanf("%d", &cusid);
                write(sockfd, &cusid, sizeof(int));

                int res;
                read(sockfd, &res, sizeof(int));
                if (res == -1){
                    printf("Invalid customer id\n");
                    continue;
                }

                struct cart c;
                read(sockfd, &c, sizeof(struct cart));

                int ordered, instock, price;
                for (int i=0; i<MAX_PROD; i++){
                    read(sockfd, &ordered, sizeof(int));
                    read(sockfd, &instock, sizeof(int));
                    read(sockfd, &price, sizeof(int));
                    printf("Product id- %d\n", c.products[i].id);
                    printf("Ordered - %d; In stock - %d; Price - %d\n", ordered, instock, price);
                    c.products[i].qty = instock;
                    c.products[i].price = price;
                }

                int total = calculateTotal(c);
                
                printf("Total in your cart\n");
                printf("%d\n", total);
                int payment;

                while (1){
                    printf("Please enter the amount to pay\n");
                    scanf("%d", &payment);

                    if (payment != total){
                        printf("Wrong total entered, enter again\n");
                    }else{
                        break;
                    }
                }

                char ch = 'y';
                printf("Payment recorded, order placed");
                write(sockfd, &ch, sizeof(char));
                generateReceipt(total, c);
            }

            else if (ch == 'g'){
                char conf;
                printf("Press y/n if you want to continue\n");
                scanf("%c", &conf);

                write(sockfd, &conf, sizeof(char));
                if (conf == 'y'){

                    int id;
                    read(sockfd, &id, sizeof(int));
                    printf("Your new customer id : %d\n", id);
                    
                }else{
                    printf("Request aborted\n");
                }
            }
            else{
                printf("Invalid choice, try again\n");
            }

            displayMenuUser();
        }
    }
    else if (user == 2){
        displayMenuAdmin();

        char ch;
        scanf("%c",&ch);
        scanf("%c",&ch);
        while (1){
            write(sockfd, &ch, sizeof(ch));

            if (ch == 'a'){
                //add a product
                int id, qty, price;
                char name[50];

                printf("Enter product name\n");
                scanf("%s", name);
                // int n1 = write(sockfd, name, sizeof(name));
                // printf("%d\n", n1);
                printf("Enter product id\n");
                scanf("%d", &id);
                // write(sockfd, &id, sizeof(int));
                printf("Enter quantity\n");
                scanf("%d", &qty);
                // write(sockfd, &qty, sizeof(int));
                printf("Enter price \n");
                scanf("%d", &price);
                // write(sockfd, &price, sizeof(int));
                
                struct product p;
                p.id = id;
                strcpy(p.name, name);
                p.qty = qty;
                p.price = price;

                // printf("%ld", sizeof(struct product));
                int n1 = write(sockfd, &p, sizeof(struct product));
                // printf("%d\n", n1);

                char response[80];
                int n = read(sockfd, response, sizeof(response));
                response[n] = '\0';

                printf("%s", response);
            }

            else if (ch == 'b'){
                printf("Enter product id to be deleted\n");
                int id;
                scanf("%d", &id);
                write(sockfd, &id, sizeof(int));
                //deleting is equivalent to setting everything as -1

                char response[80];
                read(sockfd, response, sizeof(response));
                printf("%s\n", response);
            }

            else if (ch == 'c'){
                printf("Enter product id whose price is to be modified\n");
                int id;
                scanf("%d", &id);

                int price = -1;
                while (1){
                    printf("Enter new price\n");
                    scanf("%d", &price);

                    if (price <= 0){
                        printf("Price has to be > 0, try again");
                    }else{
                        break;
                    }
                }
                

                struct product p;
                p.id = id;
                p.price = price;
                write(sockfd, &p, sizeof(struct product));

                char response[80];
                read(sockfd, response, sizeof(response));
                printf("%s\n", response);
            }

            else if (ch == 'd'){
                printf("Enter product id whose quantity is to be modified\n");
                int id;
                scanf("%d", &id);

                int qty = -1;
                while (1){
                    printf("Enter new quantity\n");
                    scanf("%d", &qty);

                    if (qty < 0){
                        printf("Qunatity has to be >= 0, try again");
                    }else{
                        break;
                    }
                }

                struct product p;
                p.id = id;
                p.qty = qty;
                write(sockfd, &p, sizeof(struct product));

                char response[80];
                read(sockfd, response, sizeof(response));
                printf("%s\n", response);

            }

            else if (ch == 'e'){
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

            else if (ch == 'f'){
                break;
            }

            else{
                printf("Invalid choice, try again\n");
                scanf("%c", &ch);
                continue;
            }

            displayMenuAdmin();
            scanf("%c", &ch);
            scanf("%c", &ch);
        }
    }

    printf("Exiting program\n");
    close(sockfd);
    return 0;
}