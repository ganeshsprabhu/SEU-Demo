int p_prime(int x , int y )
{
  int output ;
  int count ;
  {
  output = 4;
  count = 0;
  while (count < 7) {
    if (x > 10) {
      {
      simulate_seu_main(& y);
      if (y == 1) {
        output = 2;
      } else {
        output = 1;
      }
      }
    } else {
      output ++;
    }
    count ++;
  }
  return (output);
}
}
