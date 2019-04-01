#!/usr/bin/ruby
# Take stdin and output the table
rows = []
cols = []
tab = {}
cur = ""
n = 0
STDIN.each_line do |line|
  puts line
  if md = /[+]+([0-9a-zA-Z-]+)/.match(line)
     rows.push(cur=line[md.begin(1)...md.end(1)])
     n += 1
  elsif md = /([0-9a-zA-Z-]+)\s*:\s*(\d+.\d+)s/.match(line)
     name=line[md.begin(1)...md.end(1)]
     cols.push(name) if n == 1
     tab[cur + name] = line[md.begin(2)...md.end(2)]
  end
end


def print_header(cols, mr, mc)
  print "|".ljust(mr, " ")
  cols.each do |e|
    print " | ", e.ljust(mc, " ")
  end
  print " |\n", ":".ljust(mr + 1, "-")
  cols.each do |e|
    print "|", ":".rjust(mc + 2, "-")
  end
  print "|\n"
end

mr = rows.map { |e| e.length}.max
mc = cols.map { |e| e.length}.max
mc = 11 if mc < 11

print_header(cols, mr, mc)

rows.each { |r|
  print "|", r.ljust(mr, " "), "| "
  min = 100000.0;
  cols.each { |c|
    min = tab[r + c].to_f if tab.has_key?(r + c) && tab[r + c].to_f < min
  }
  cols.each { |c|
    if ! tab.has_key?(r + c)
      print "-".ljust(mc - 4, " "), " | "
      continue
    end
    v = tab[r + c]
    print v.to_f == min ? "**" : "  "
    print v
    print v.to_f == min ? "**" : "  "
    print " ".ljust(mc - 4 - v.length, " ")
    print " | "
  }
  print "\n"
}
