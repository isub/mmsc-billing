ORACLE_HOME = /oracle/orahome
ORA_H = -I$(ORACLE_HOME)/rdbms/public
ORA_LIB_DIR = -L$(ORACLE_HOME)/lib
ORA_LIB = -lclntsh
UTL_DIR = ../../utils
UTL_H = -I../..
MBUNI_H = -I/home/mmsc/mmsc_src/mbuni-1.6.0
MBUNI_UTL_H = -I/home/mmsc/mmsc_src/mbuni-1.6.0/mmlib
KANNEL_H = -I/usr/local/include/kannel
KANNEL_LIB_DIR = -L/usr/local/lib/kannel
KANNEL_LIB = -lgwlib
LIBXML_H = -I/home/mmsc/mmsc_src/libxml2-2.7.6/include

mmsc-billing: ./obj/mmsc-billing.o ./obj/dbpool.o ./obj/billing-auxiliary.o ./obj/config.o ./obj/log.o
	gcc -shared -o ../libmmsc-billing.so $(KANNEL_LIB_DIR) $(KANNEL_LIB) ./obj/mmsc-billing.o ./obj/dbpool.o ./obj/billing-auxiliary.o ./obj/config.o ./obj/log.o $(ORA_LIB_DIR) $(ORA_LIB) -lstdc++
./obj/mmsc-billing.o: mmsc-billing.c ./obj/billing-auxiliary.o ./obj/dbpool.o
	gcc -fPIC -c mmsc-billing.c -o ./obj/mmsc-billing.o $(UTL_H) $(MBUNI_H) $(MBUNI_UTL_H) $(KANNEL_H) $(LIBXML_H)
./obj/billing-auxiliary.o: billing-auxiliary.cpp
	gcc -fPIC -c billing-auxiliary.cpp $(UTL_H) -o ./obj/billing-auxiliary.o $(ORA_H) $(UTL_H) $(MBUNI_H) $(KANNEL_H) $(LIBXML_H)
./obj/dbpool.o: $(UTL_DIR)/dbpool/dbpool.cpp
	g++ -fPIC -c $(UTL_DIR)/dbpool/dbpool.cpp $(UTL_H) $(ORA_H) -o ./obj/dbpool.o
./obj/config.o: $(UTL_DIR)/config/config.cpp
	g++ -fPIC -c $(UTL_DIR)/config/config.cpp -o ./obj/config.o
./obj/log.o: $(UTL_DIR)/log/log.cpp
	g++ -fPIC -c $(UTL_DIR)/log/log.cpp -o ./obj/log.o

install:
	cp -f ../*.so /usr/local/lib

clean:
	rm -f ./obj/* ../*.so