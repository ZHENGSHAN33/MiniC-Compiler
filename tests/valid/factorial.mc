int main() {
    int x;
    int fact;
    read(x);
    fact = 1;
    while (x > 0) {
        fact = fact * x;
        x = x - 1;
    }
    write(fact);
    return 0;
}
