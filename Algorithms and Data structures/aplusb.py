file_in = open("aplusb.in", "r")
file_out = open("aplusb.out", "w")
a, b = map(int, file_in.readline().split())
print(a + b, file = file_out)
file_out.close()