# make sure that correct version of openstudio is loaded before running this, run from cli like:
#
# openstudio.exe UpdateHVACLibrary.rb

require 'openstudio'
require 'etc'
# gem install parallel
# gem install ruby-progressbar
require 'parallel'
require_relative 'OSMVersionsLib.rb'

# Environment variables
if ENV['N'].nil?
  # Number of parallel runs caps to nproc - 1
  nproc = [1, Etc.nprocessors - 1].max
  puts "Defaulted Nproc to #{nproc}"
else
  nproc = ENV['N'].to_i
end

# If CTRL+C is pressed, kill every sub-processes
trap("TERM") do
  puts "KILLING"
  raise Parallel::Kill
  exit!
end


start_time = Time.now

os_version_str = parse_version_from_cmake(SDK_CMAKE_PATH)
puts "FindOpenStudioSDK.cmake has OpenStudio at version #{os_version_str}"
current_os_version = OpenStudio::openStudioVersion
if (os_version_str != current_os_version)
  raise "Current openstudio used is as version #{current_os_version} while FindOpenStudioSDK.cmake has it at #{os_version_str}"
end

if !OpenStudio::openStudioVersionPrerelease.empty?
  puts ""
  puts "*" * 80
  puts "   /!\\ Warning /!\\ You are trying to upgrade the libraries\n"
  puts "                   with a pre-release installer. Are you sure?!"
  puts "*" * 80
  puts ""
end


all_osms = find_resource_osms()

mismatches = check_all_osm_versions(all_osms, os_version_str)

Parallel.map(mismatches,
             in_process: nproc,
             progress: "Updating Libraries") do |model_path|

  puts "Starting for '#{model_path}'"

  model_path = OpenStudio::Path.new(model_path)
  #model_path = OpenStudio::Path.new('hvac_library.osm')

  # Load the model, version translating if necessary
  if OpenStudio::exists(model_path)
    versionTranslator = OpenStudio::OSVersion::VersionTranslator.new
    model = versionTranslator.loadModel(model_path)
    if model.empty?
      puts "Version translation failed for #{model_path}"
      exit
    else
      model = model.get
    end
  else
    puts "The model couldn't be found for #{model_path}"
    exit
  end

  # Save updated model
  model.save(model_path,true)
end

# Show the timing
end_time = Time.now
total_time_min = ((end_time - start_time)/60.0).round(1)
puts "*** Finished Updating #{mismatches.size} files at: #{end_time}, time elapsed = #{total_time_min} min."
