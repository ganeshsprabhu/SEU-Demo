int p_prime(int x , int y )
{
  int output ;
  int count ;
  {
  output = 4;
  count = 0;
  while (count < 7) {
    if (x > 10) {
      if (y == 1) {
        simulate_seu_main(& output);
        output = 2;
      } else {
        simulate_seu_main(& output);
        output = 1;
      }
    } else {
      simulate_seu_main(& output);
      output ++;
    }
    count ++;
  }
  {
  simulate_seu_main(& output);
  return (output);
  }
}
}
