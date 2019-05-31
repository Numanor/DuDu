# - * - coding: utf-8 - * -

with open("./_road.nedge", 'r') as f:
    origin = f.readlines()

print(len(origin))

for idx in range(len(origin)):
    origin[idx] = str(idx) + ' ' + origin[idx]

with open("./road.cedge", 'w') as f:
    f.writelines(origin)

with open("./car.txt", 'r') as f:
    origin = f.readlines()

for idx in range(len(origin)):
    nodeidx = origin[idx].split(' ')[2].split(',')[2].rstrip('\n')
    # print(nodeidx)
    origin[idx] = nodeidx + ' ' + str(idx) + '\n'
    # print(origin[idx])

with open("./cab.object", 'w') as f:
    f.writelines(origin)
