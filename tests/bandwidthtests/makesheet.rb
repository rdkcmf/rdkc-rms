##########################################################################
# If not stated otherwise in this file or this component's LICENSE
# file the following copyright and licenses apply:
#
# Copyright 2019 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################!/usr/bin/env ruby -w -s
# -*- coding: utf-8 -*-
$LOAD_PATH.unshift "#{File.dirname(__FILE__)}/../lib"

# convert performance test data from .csv to .xlsx with charts

require "csv"
require 'axlsx'

def replace_header(header)
  case header
    when /_count/
      header.gsub!("_count", "count")
    when /_cpue/
      header.gsub!("_cpue", "cpu")
    when /_meme/
      header.gsub!("_meme", "mem")
    when /_cpu/
      header.gsub!("_cpu", "cpu")
    when /_mem/
      header.gsub!("_mem", "mem")
    when /_rx_/
      header.gsub!("_rx_", "RX")
    when /_tx_/
      header.gsub!("_tx_", "TX")
    when /_sum_/
      header.gsub!("_sum_", "SUM")
  end
  case header
    when /w1/
      header.gsub!("w1", "EWS")
    when /r1/
      header.gsub!("r1", "RUBY")
    when /sum/
      header.gsub!("sum", "SUM")
    when /all/
      header.gsub!("all", "ALL")
    when '_'
      header = 'note'
  end
  return header
end

def add_series_to_chart(sheet, chart, column, lines, color)
  cell_header = Axlsx::cell_r(column, 0)
  cell_start = Axlsx::cell_r(column, 1)
  cell_end = Axlsx::cell_r(column, lines - 1)
  chart.add_series :xData => sheet["A2:A#{lines - 1}"], :yData => sheet["#{cell_start}:#{cell_end}"],
      :title => sheet["#{cell_header}"], :color => color
end

def get_column_max(column, lines)
  cell_start = Axlsx::cell_r(column, 1)
  cell_end = Axlsx::cell_r(column, lines - 1)
  return "=max(#{cell_start}:#{cell_end})"
end

begin

  # all csv files
  csv_files = Dir.glob("*.csv")
  csv_files.each do |filename|

    # open csv file
    csv_file_path = File.join(File.dirname(__FILE__), filename)
    print "#{csv_file_path}: "

    # open sheet
    package = Axlsx::Package.new
    wb = package.workbook
    wb.add_worksheet(:name => "performance") do |sheet|

      # read csv row, add sheet row
      headers = []
      items = []
      lines = 0
      CSV.foreach(csv_file_path, :headers => true, :header_converters => :symbol, :converters => :numeric) do |row|
        if headers.size == 0
          headers = row.headers
          headers.size.times do |i|
            header = headers[i].to_s.downcase
            header = replace_header(header)
            headers[i] = header.to_sym
          end
          sheet.add_row headers
        else
          items = row.collect { |k, v| v }
          sheet.add_row items
        end
        lines += 1
        print "#{lines} "
      end
      puts

      # map header to column
      column = {}
      headers.size.times do |i|
        column[headers[i]] = i
      end

      # add row with max values per column
      if headers.size != 0
        column_max = []
        headers.size.times do |i|
          if i == 0
            column_max[i] = "MAX"
          else
            column_max[i] = get_column_max(i, lines)
          end
        end
        sheet.add_row column_max
      end

      # generate stream count chart
      sheet.add_chart(Axlsx::ScatterChart, :title => "STREAM COUNT") do |chart|
        chart.start_at 0, lines + 1
        chart.end_at 20, lines + 16
        add_series_to_chart(sheet, chart, column[:count], lines, "FF0000")
        chart.x_val_axis.cross_axis.title = 'count'
        chart.x_val_axis.title = 'seconds'
      end

      # generate cpu usage chart
      sheet.add_chart(Axlsx::ScatterChart, :title => "CPU USAGE") do |chart|
        chart.start_at 0, lines + 17
        chart.end_at 20, lines + 32
        add_series_to_chart(sheet, chart, column[:cpuALL], lines, "808080")
        add_series_to_chart(sheet, chart, column[:cpuSUM], lines, "0000FF")
        add_series_to_chart(sheet, chart, column[:cpuRUBY], lines, "00FF00")
        add_series_to_chart(sheet, chart, column[:cpuEWS], lines, "FF0000")
        column[:cpu1].upto(column[:cpuEWS] - 1) do |col|
          add_series_to_chart(sheet, chart, col, lines, "808000")
        end
        chart.x_val_axis.cross_axis.title = 'CPU %'
        chart.x_val_axis.title = 'seconds'
      end

      # generate mem usage chart
      sheet.add_chart(Axlsx::ScatterChart, :title => "MEM USAGE") do |chart|
        chart.start_at 0, lines + 33
        chart.end_at 20, lines + 48
        add_series_to_chart(sheet, chart, column[:memRUBY], lines, "00FF00")
        add_series_to_chart(sheet, chart, column[:memEWS], lines, "FF0000")
        column[:mem1].upto(column[:memEWS] - 1) do |col|
          add_series_to_chart(sheet, chart, col, lines, "808000")
        end
        chart.x_val_axis.cross_axis.title = 'MEM %'
        chart.x_val_axis.title = 'seconds'
      end

      # generate bandwidth chart
      sheet.add_chart(Axlsx::ScatterChart, :title => "BANDWIDTH") do |chart|
        chart.start_at 0, lines + 49
        chart.end_at 20, lines + 64
        add_series_to_chart(sheet, chart, column[:SUMkbps], lines, "FF0000")
        add_series_to_chart(sheet, chart, column[:TXkbps], lines, "0000FF")
        add_series_to_chart(sheet, chart, column[:RXkbps], lines, "00FF00")
        chart.x_val_axis.cross_axis.title = 'kbps'
        chart.x_val_axis.title = 'seconds'
      end

    end

    # write xls file
    filename.gsub!(".csv", ".xlsx")
    package.serialize(filename)

  end
end
