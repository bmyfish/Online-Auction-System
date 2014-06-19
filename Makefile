all: auctionserver bidder1 bidder2  seller1 seller2

auctionserver: auctionserver.c
	gcc -o auctionserver auctionserver.c -lnsl -lsocket -lresolv

bidder1: bidder1.c
	gcc -o bidder1 bidder1.c -lnsl -lsocket -lresolv

bidder2: bidder2.c
	gcc -o bidder2 bidder2.c -lnsl -lsocket -lresolv

seller1: seller1.c
	gcc -o seller1 seller1.c -lnsl -lsocket -lresolv

seller2: seller2.c
	gcc -o seller2 seller2.c -lnsl -lsocket -lresolv

clean:
	rm -f *.o auctionserver bidder1 bidder2 seller1 seller2
