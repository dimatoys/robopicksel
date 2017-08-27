
def loadAnomalyMatrix(fileName):
    data = {}
    maxX = -1
    maxY = -1
    maxC = -1
    file = open(fileName, 'r')
    while 1:
        line = file.readline()
        if line:
            lineArr = line.split()
            if len(lineArr) == 4:
                x = int(lineArr[0])
                y = int(lineArr[1])
                c = int(lineArr[2])
                v = float(lineArr[3])
                data[(x,y,c)] = v
                if x > maxX:
                    maxX = x
                if y > maxY:
                    maxY = y
                if c > maxC:
                    maxC = c
        else:
            break
    file.close()

    anomalyArr = []
    for y in range(maxY + 1):
        line = []
        for x in range(maxX + 1):
            cell = []
            for c in range(maxC + 1):
                cell.append(data[(x,y,c)])
            line.append(cell)
        anomalyArr.append(line)

    return anomalyArr

def printLine(anomalyArr, y):
    line = anomalyArr[y]
    for cell in line:
        print cell[0], cell[1], cell[2]

def printSmoothes(lines, color, maxs):
    num = len(lines)
    length = len(lines[0])
    sums = []
    smooth = []
    for i in range(num):
        line = []
        for j in range(maxs):
            smooth.append([])
            line.append(0)
        sums.append(line)
    #print num, length
    for i in range(length):
        for j in range(num):
            for k in range(maxs):
                #old = sums[j][k]
                sums[j][k] = sums[j][k] + lines[j][i][color]
                #print "i=%d sums[j=%d][k=%d] = %d + %d = %d" % (i,j,k,old,lines[j][i][color], sums[j][k])
                if (i + 1) % (k + 1) == 0:
                    smooth[j + num * k].append(sums[j][k] / (k + 1))
                    #print "i=%d j=%d k=%d smooth[%d] <- %d / %d smooth=%s" % (i,j,k,j * maxs + k, sums[j][k], k + 1, str(smooth))
                    sums[j][k] = 0
    
    #print smooth
    
    for i in range(len(smooth[0])):
        separator = ""
        lineStr = ""
        for j in range(maxs * num):
            #print "i=%d j=%d len=%d" % (i,j,len(smooth[j]))
            if i < len(smooth[j]):
                lineStr = "%s%s%f" % (lineStr, separator, smooth[j][i])
                separator = ","
            else:
                break
        print lineStr

anomalyArr = loadAnomalyMatrix('anomalyDump1.txt')

#printLine(anomalyArr,121)
#printSmoothes([anomalyArr[120], anomalyArr[121], anomalyArr[122]], 0, 10)
#printSmoothes([anomalyArr[120]], 0, 2)
#printSmoothes([[[1.0,0,0],[2.0,0,0],[3.0,0,0],[4.0,0,0]],[[5.0,0,0],[6.0,0,0],[7.0,0,0],[8.0,0,0]]], 0, 2)
