int main() {
    int a;
    int b;
    bool ok;
    a = 2 + 3 * 4;
    b = (2 + 3) * 4;
    ok = a < b && true || false;
    if (ok) {
        while (a < b) {
            a = a + 1;
        }
    } else {
        b = b - 1;
    }
    write(a);
    return 0;
}
