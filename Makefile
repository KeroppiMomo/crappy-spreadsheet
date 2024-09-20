CC = g++
FLAGS = -std=c++17 -I /opt/homebrew/Cellar/boost/1.86.0/include/

.PHONY: clean

compilation: main.o terminal.o worksheet_reference.o expression.o worksheet.o workspace.o
	$(CC) $(FLAGS) -o compilation $^

main.o: main.cpp
	$(CC) $(FLAGS) -c main.cpp -o $@

terminal.o: terminal.cpp terminal.h
	$(CC) $(FLAGS) -c terminal.cpp -o $@

worksheet_reference.o: worksheet_reference.cpp worksheet_reference.h
	$(CC) $(FLAGS) -c worksheet_reference.cpp -o $@

expression.o: expression.cpp expression.h worksheet.h workspace.h
	$(CC) $(FLAGS) -c expression.cpp -o $@

worksheet.o: worksheet.cpp worksheet.h terminal.h worksheet_reference.h expression.h
	$(CC) $(FLAGS) -c worksheet.cpp -o $@

workspace.o: workspace.cpp workspace.h worksheet.h terminal.h worksheet_reference.h expression.h
	$(CC) $(FLAGS) -c workspace.cpp -o $@

clean:
	rm *.o compilation

