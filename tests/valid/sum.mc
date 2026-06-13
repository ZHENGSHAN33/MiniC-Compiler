int main() {
    int n;
    int i;
    int s;
    read(n);
    i = 1;
    s = 0;
    while (i <= n) {
        s = s + i;
        i = i + 1;
    }
    write(s);
    return 0;
}
