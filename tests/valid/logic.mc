int main() {
    int x;
    bool ok;
    read(x);
    ok = x > 0 && x < 10;
    if (ok) {
        write(1);
    } else {
        write(0);
    }
    return 0;
}
