#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include "headers.h"

int main(){
    printf("Setting up server\n");

    //file containing all the records is called records.txt

    int fd = open("records.txt", O_RDWR | O_CREAT, 0777);
    int fd_cart = open("orders.txt", O_RDWR | O_CREAT, 0777);
    int fd_custs = open("customers.txt", O_RDWR | O_CREAT, 0777);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1){
        perror("Error: ");
        return -1;
    }

    struct sockaddr_in serv, cli;
    
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(5555);

    int opt = 1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))){
        perror("Error: ");
        return -1;
    }

    if (bind(sockfd, (struct sockaddr *)&serv, sizeof(serv)) == -1){
        perror("Error: ");
        return -1;
    }

    if (listen(sockfd, 5) == -1){
        perror("Error: ");
        return -1;
    }

    int size = sizeof(cli);
    printf("Server set up successfully\n");

    while (1){

        int new_fd = accept(sockfd, (struct sockaddr *)&cli, &size);
        if (new_fd == -1){
            perror("Error: ");
            return -1;
        }

        if (!fork()){
            printf("Connection with client successful\n");
            close(sockfd);

            char ch;
            while (1){
                read(new_fd, &ch, sizeof(char));

                lseek(fd, 0, SEEK_SET);
                lseek(fd_cart, 0, SEEK_SET);
                lseek(fd_custs, 0, SEEK_SET);

                if (ch == 'a'){
                    printf("Terminating connection\n");
                    close(new_fd);
                    break;
                }
                
                else if (ch == 'b'){

                    struct flock lock;
                    lock.l_whence = SEEK_SET;
                    lock.l_start = 0;
                    lock.l_len = 0;
                    lock.l_type = F_RDLCK;

                    printf("Retrieving data\n");
                    fcntl(fd, F_SETLKW, &lock);

                    struct product p;
                    while (read(fd, &p, sizeof(struct product))){
                        write(new_fd, &p, sizeof(struct product));
                    }
                    p.id = -1;
                    write(new_fd, &p, sizeof(struct product));

                    lock.l_type = F_UNLCK;
                    fcntl(fd, F_SETLKW, &lock);

                }
                else if (ch == 'c'){
                    int cust_id = -1;
                    read(new_fd, &cust_id, sizeof(int));

                    struct flock lock_cart;
                    
                    int i = -1;
                    lock_cart.l_whence = SEEK_SET;
                    lock_cart.l_len = sizeof(struct cart);
                    lock_cart.l_type = F_RDLCK;
                    struct cart c;

                    struct flock lock_cust;
                    lock_cust.l_len = 0;
                    lock_cust.l_type = F_RDLCK;
                    lock_cust.l_start = 0;
                    lock_cust.l_whence = SEEK_SET;

                    fcntl(fd_custs, F_SETLKW, &lock_cust);

                    int id = -1;
                    int flg = 0;
                    while (read(fd_custs, &id, sizeof(int))){
                        if (id == cust_id){
                            flg = 1;
                            break;
                        }
                    }

                    if (!flg){
                        struct cart c;
                        c.custid = -1;
                        write(new_fd, &c, sizeof(struct cart));
                        
                    }else{
                        struct cart c;
                        lock_cart.l_start = id*sizeof(struct cart);

                        fcntl(fd_cart, F_SETLKW, &lock_cart);
                        lseek(fd_cart, i*sizeof(struct cart), SEEK_SET);
                        read(fd_cart, &c, sizeof(struct cart));
                        write(new_fd, &c, sizeof(struct cart));
                    }

                    lock_cart.l_type = F_UNLCK;
                    lock_cust.l_type = F_UNLCK;
                    fcntl(fd_cart, F_SETLKW, &lock_cart);
                    fcntl(fd_custs, F_SETLKW, &lock_cust);
                }

                else if (ch == 'd'){
                    
                    int cust_id = -1;
                    read(new_fd, &cust_id, sizeof(int));

                    struct flock lock_cust;
                    lock_cust.l_len = 0;
                    lock_cust.l_type = F_RDLCK;
                    lock_cust.l_start = 0;
                    lock_cust.l_whence = SEEK_SET;

                    fcntl(fd_custs, F_SETLKW, &lock_cust);

                    int id = -1;
                    int flg = 0;
                    while (read(fd_custs, &id, sizeof(int))){
                        if (id == cust_id){
                            flg = 1;
                            break;
                        }
                    }

                    int val = -1;
                    if (!flg){
                        write(new_fd, &val, sizeof(int));
                        lock_cust.l_type = F_UNLCK;
                        fcntl(fd_custs, F_SETLKW, &lock_cust);
                        continue;
                    }

                    int noprod;
                    read(new_fd, &noprod, sizeof(int));

                    struct flock lock_cart;
                    
                    int i = -1;
                    lock_cart.l_whence = SEEK_SET;
                    lock_cart.l_len = sizeof(struct cart);
                    lock_cart.l_start = cust_id * sizeof(struct cart);
                    lock_cart.l_type = F_WRLCK;

                    struct flock lock_prod;
                    lock_prod.l_whence = SEEK_SET;
                    lock_prod.l_start = 0;
                    lock_prod.l_len = 0;
                    lock_prod.l_type = F_RDLCK;

                    fcntl(fd_cart, F_SETLKW, &lock_cart);
                    struct cart c;

                    lseek(fd_cart, cust_id * sizeof(struct cart), SEEK_SET);
                    read(fd_cart, &c, sizeof(struct cart));

                    for (int i=0; i<noprod; i++){
                        struct product p;
                        read(new_fd, &p, sizeof(struct product));

                        int prod_id = p.id;
                        int qty = p.qty;

                        if (qty < 0){
                            write(new_fd, "Quantity can't be negative\n", sizeof("Qunatity can't be negative\n"));
                        }

                        struct product p1;
                        int found = 0;
                        while (read(fd, &p1, sizeof(struct product))){
                            if (p1.id == p.id) {
                                if (p1.qty >= p.qty){

                                    p1.qty -= p.qty;
                                    found = 1;
                                    break;
                                }
                            }
                        }

                        if (!found){
                            write(new_fd, "Product id invalid or out of stock\n", sizeof("Product id invalid or out of stock\n"));
                            lock_cart.l_type = F_UNLCK;
                            lock_cust.l_type = F_UNLCK;
                            lock_prod.l_type = F_UNLCK;

                            fcntl(fd, F_SETLKW, &lock_prod);
                            fcntl(fd_cart, F_SETLKW, &lock_cart);
                            fcntl(fd_custs, F_SETLKW, &lock_cust);
                            continue;
                        }

                        int flg = 0;
                        
                        int flg1 = 0;
                        for (int i=0; i<MAX_PROD; i++){
                            if (c.products[i].id == p.id){
                                flg1 = 1;
                                break;
                            }
                        }

                        if (flg1){
                            write(new_fd, "Product already exists in cart\n", sizeof("Product already exists in cart\n"));
                            lock_cart.l_type = F_UNLCK;
                            lock_cust.l_type = F_UNLCK;
                            lock_prod.l_type = F_UNLCK;

                            fcntl(fd, F_SETLKW, &lock_prod);
                            fcntl(fd_cart, F_SETLKW, &lock_cart);
                            fcntl(fd_custs, F_SETLKW, &lock_cust);
                            continue;
                        }

                        for (int i=0; i<MAX_PROD; i++){
                            if (c.products[i].id != -1){
                                flg = 1;
                                c.products[i].id = p.id;
                                c.products[i].qty = p.qty;
                                strcpy(c.products[i].name, p1.name);
                                c.products[i].price = p1.price;
                            }else if (c.products[i].qty <= 0){
                                flg = 1;
                                c.products[i].id = p.id;
                                c.products[i].qty = p.qty;
                                strcpy(c.products[i].name, p1.name);
                                c.products[i].price = p1.price;
                            }
                        }

                        if (!flg){
                            write(new_fd, "Cart limit reached\n", sizeof("Cart limit reached\n"));
                            lock_cart.l_type = F_UNLCK;
                            lock_cust.l_type = F_UNLCK;
                            lock_prod.l_type = F_UNLCK;

                            fcntl(fd, F_SETLKW, &lock_prod);
                            fcntl(fd_cart, F_SETLKW, &lock_cart);
                            fcntl(fd_custs, F_SETLKW, &lock_cust);
                            continue;
                        }

                        write(new_fd, "Item added to cart\n", sizeof("Item added to cart\n"));
                    }

                    lseek(fd_cart, (cust_id)*sizeof(struct cart), SEEK_SET);
                    write(fd_cart, &c, sizeof(struct cart));
                    // write(new_fd, "Valid products added to cart\n", sizeof("Valid products added to cart\n"));
                    lock_cart.l_type = F_UNLCK;
                    lock_cust.l_type = F_UNLCK;
                    lock_prod.l_type = F_UNLCK;

                    fcntl(fd, F_SETLKW, &lock_prod);
                    fcntl(fd_cart, F_SETLKW, &lock_cart);
                    fcntl(fd_custs, F_SETLKW, &lock_cust);

                }

                else if (ch == 'e'){

                    int cust_id = -1;
                    read(new_fd, &cust_id, sizeof(int));

                    struct flock lock_cust;
                    lock_cust.l_len = 0;
                    lock_cust.l_type = F_RDLCK;
                    lock_cust.l_start = 0;
                    lock_cust.l_whence = SEEK_SET;

                    fcntl(fd_custs, F_SETLKW, &lock_cust);

                    int id = -1;
                    int flg = 0;
                    while (read(fd_custs, &id, sizeof(int))){
                        if (id == cust_id){
                            flg = 1;
                            break;
                        }
                    }

                    int val = -1;
                    if (!flg){

                        write(new_fd, &val, sizeof(int));
                        continue;
                        lock_cust.l_type = F_UNLCK;
                        fcntl(fd_custs, F_SETLKW, &lock_cust);

                    }

                    struct flock lock_cart;
                    
                    int i = -1;
                    lock_cart.l_whence = SEEK_SET;
                    lock_cart.l_len = sizeof(struct cart);
                    lock_cart.l_start = cust_id * sizeof(struct cart);
                    lock_cart.l_type = F_WRLCK;
                    fcntl(fd_cart, F_SETLKW, &lock_cart);

                    struct cart c;
                    lseek(fd_cart, cust_id*sizeof(struct cart), SEEK_SET);
                    read(fd_cart, &c, sizeof(struct cart));

                    int pid, qty;
                    struct product p;
                    read(new_fd, &p, sizeof(struct product));

                    pid = p.id;
                    qty = p.qty;

                    int flg = 0;
                    for (int i=0; i<MAX_PROD; i++){
                        if (c.products[i].id == pid){
                            
                            struct flock lock_prod;
                            lock_prod.l_whence = SEEK_SET;
                            lock_prod.l_len = 0;
                            lock_prod.l_start = 0;
                            lock_prod.l_type = F_RDLCK;

                            fcntl(fd, F_SETLKW, &lock_prod);

                            struct product p1;
                            while (read(fd, &p1, sizeof(struct product))){
                                if (p1.id == pid) {
                                    if (p1.qty >= qty){
                                        flg = 1;
                                        break;
                                    }else{
                                        flg = 0;
                                        break;
                                    }
                                }
                            }

                            lock_prod.l_type = F_UNLCK;
                            fcntl(fd, F_SETLKW, &lock_prod);

                            break;
                        }
                    }

                    if (!flg){
                        write(new_fd, "Product id not in the order or out of stock\n", sizeof("Product id not in the order or out of stock\n"));
                        continue;
                    }

                    c.products[i].qty = qty;
                    write(new_fd, "Update successful\n", sizeof("Update successful\n"));
                    lseek(fd_cart, cust_id*sizeof(struct cart), SEEK_SET);
                    write(fd_cart, &c, sizeof(struct cart));

                    lock_cart.l_type = F_UNLCK;
                    lock_cust.l_type = F_UNLCK;
                    fcntl(fd_cart, F_SETLKW, &lock_cart);
                    fcntl(fd_custs, F_SETLKW, &lock_cust);

                }

                else if (ch == 'f'){
                    int cust_id = -1;
                    read(new_fd, &cust_id, sizeof(int));

                    struct flock lock_cust;
                    lock_cust.l_len = 0;
                    lock_cust.l_type = F_RDLCK;
                    lock_cust.l_start = 0;
                    lock_cust.l_whence = SEEK_SET;

                    fcntl(fd_custs, F_SETLKW, &lock_cust);

                    int id = -1;
                    int flg = 0;
                    while (read(fd_custs, &id, sizeof(int))){
                        if (id == cust_id){
                            flg = 1;
                            break;
                        }
                    }

                    int val = -1;
                    if (!flg){
                        write(new_fd, &val, sizeof(int));
                        continue;

                        lock_cust.l_type = F_UNLCK;
                        fcntl(fd_custs, F_SETLKW, &lock_cust);
                    }

                    struct cart c;
                    lseek(fd_cart, cust_id*sizeof(struct cart), SEEK_SET);
                    read(fd_cart, &c, sizeof(struct cart));
                    write(new_fd, &c, sizeof(struct cart));

                    int total = 0;

                    for (int i=0; i<MAX_PROD; i++){
                        write(new_fd, &c.products[i].qty, sizeof(int));

                        lseek(fd, 0, SEEK_SET);

                        struct product p;
                        while (read(fd, &p, sizeof(struct product))){

                            if (p.id == c.products[i].id) {
                                int min ;
                                if (c.products[i].qty >= p.qty){
                                    min = p.qty;
                                }else{
                                    min = c.products[i].qty;
                                }

                                write(new_fd, &min, sizeof(int));
                                write(new_fd, &p.qty, sizeof(int));
                            }
                        }
                    }
                    char ch;
                    read(new_fd, &ch, sizeof(char));

                    lseek(fd_cart, cust_id*sizeof(struct cart), SEEK_SET);

                    for (int i=0; i,MAX_PROD; i++){
                        c.products[i].id = -1;
                        strcpy(c.products[i].name, "");
                        c.products[i].price = -1;
                        c.products[i].qty = -1;
                    }

                    write(fd_cart, &c, sizeof(struct cart));
                }

                else if (ch == 'g'){
                    char buf;
                    read(new_fd, &buf, sizeof(char));
                    if (buf == 'y'){

                        struct flock lock;
                        lock.l_whence = SEEK_SET;
                        lock.l_len = 0;
                        lock.l_start = 0;
                        lock.l_type = F_RDLCK;
                        fcntl(fd_custs, F_SETLKW, &lock);
                        
                        int max_id = -1; 
                        int id = -1;
                        while (read(fd_custs, &id, sizeof(int))){
                            if (id > max_id){
                                max_id = id;
                            }
                        }

                        max_id ++;
                        lock.l_type = F_UNLCK;
                        fcntl(fd_custs, F_SETLKW, &lock);
                        write(new_fd, &max_id, sizeof(int));

                    }else{
                        continue;
                    }
                }

            }
            printf("Connection terminated\n");

        }else{
            close(new_fd);
        }
    }
}