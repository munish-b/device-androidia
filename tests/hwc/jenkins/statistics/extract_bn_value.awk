{
  split($0,colonsep,":");
  split(colonsep[2],data,",");
  split(colonsep[1],path_components,"/");
  bn=path_components[7];
  if (int(bn) > 0)
  {
    printf("%s,%s\n", bn, data[4]);
  }
}