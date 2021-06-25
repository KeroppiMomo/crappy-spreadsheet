#include "terminal.h"
#include "worksheet_reference.h"
#include "worksheet.h"
#include "workspace.h"
#include <execinfo.h>
#include <unistd.h>

void handler(int sig) {
    void *array[100];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 100);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}

int main() {
    signal(SIGABRT, handler);
    // while (true) {
    //     std::string input;
    //     getline(std::cin, input);
    //     try {
    //         std::cout << expression::parse(input)->evaluate() << std::endl;
    //     } catch (expression::parse_exception e) {
    //         std::cout << "Parse exception: attempted to parse '" << e.attempt << "' but failed with message '" << e.message << "'." << std::endl;
    //     }
    // }
    // std::cout << expression::parse("sum(1, sum(100, 900, 100))")->evaluate() << std::endl;
    // return 0;

    while (true) {
        workspace::render();
        terminal::flush();

        char ch = terminal::getch();
        workspace::action(ch);
    }

    return 0;
}
