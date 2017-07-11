{
  split($0,colonsep,":");
  split(colonsep[2],data,",");
  split(colonsep[1],path_components,"/");
  file=path_components[8];
  testset=substr(file,11);
  test=data[1];
  printf("%s(%s),%s\n", test, testset, data[4]);
}
