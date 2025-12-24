int p(int x , int y )
{
  int output ;
  int count ;
  {
  output = 4;
  count = 0;
  while (count < 7) {
    if (x > 10) {
      if (y == 1) {
        output = 2;
      } else {
        output = 1;
      }
    } else {
      output ++;
    }
    count ++;
  }
  return (output);
}
}
int main(void)
{
  int output ;
  int x ;
  int y ;
  int i ;
  {
  i = 0;
  while (i < 10) {
    output = p(x, y);
    output_prime = p_prime(x_prime, y_prime);
    i ++;
  }
  return (0);
}
}
extern int ( simulate_seu_main)() ;
int p_prime(int x , int y )
{
  int output ;
  int count ;
  {
  output = 4;
  count = 0;
  while (1) {
    simulate_seu_main(& count);
    if (! (count < 7)) {
      break;
    }
    if (x > 10) {
      if (y == 1) {
        output = 2;
      } else {
        output = 1;
      }
    } else {
      output ++;
    }
    simulate_seu_main(& count);
    count ++;
  }
  return (output);
}
}
