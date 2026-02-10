void main() {
  int x = 1;
  { int x = 2; x = x + 3; print("g09i=", x, "\n"); }
  print("g09o=", x, "\n");
}
