with open("log.txt", "rt") as file:
    txt = file.read()

rows = [each for each in txt.split("\n") if each]
counts = dict()
times = dict()

for each in rows:
    row = each.split(",")
    task_num, ctime, rtime = int(row[0]), float(row[1]), float(row[2])
    count = counts.get(task_num, 0) + 1
    counts[task_num] = count
    create_time, run_time = times.get(task_num, (.0, .0))
    create_time += ctime
    run_time += rtime
    times[task_num] = (create_time, run_time)
    
print("tasks\ttimes\tctime(ms)\trtime(ms)")
for task_num, count in counts.items():
    ctime, rtime = times.get(task_num, (.0, .0))
    ctime /= count
    rtime /= count
    print("%d\t%d\t%.2f\t\t%.2f" % (task_num, count, ctime, rtime))

