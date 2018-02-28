import os
import statvfs
import stat
import tempfile 

def stripStr(obj):
	"""
	# strip whatever and always return string
	"""
	if obj is None:
		return ''
	return str(obj).strip()

def strToFloat(val, defaultValue = None):
	"""
	    convert string value to float value
	"""
	val = str(val).strip()
	try:
		return float(val)
	except ValueError:
		return defaultValue

def getFloatFromEnv(name, defaultValue = 0):
	envVal = stripStr(os.getenv(name))
	return strToFloat(envVal, defaultValue)

MB = 1024 * 1024 
GB = 1024 * 1024 * 1024
MAX_CACHE_SIZE_GB = getFloatFromEnv("HS_MAX_CACHE_SIZE_GB", 7.0)
REQUIRED_FREE_SPACE_GB = getFloatFromEnv("HS_REQUIRED_FREE_SPACE_GB", 4.0)
MAX_CACHE_SIZE = MAX_CACHE_SIZE_GB * GB 
REQUIRED_FREE_SPACE = REQUIRED_FREE_SPACE_GB * GB

def getCacheDir():
	baseDir = os.path.join(tempfile.gettempdir(), 'ccache')
	envDir = os.getenv("CCACHE_DIR", baseDir)
	#NOTE: out scons version does not support samba global cache well. We would need integration with build scons. 
	#instead use /tmp dir as before..
	if envDir.startswith('/remote/backup/'):
		envDir = baseDir
	
	return os.path.join(envDir, 'scons')


def cacheCleanup(directory):

	print "Reducing cache size... (HS_MAX_CACHE_SIZE_GB: %.1f, HS_REQUIRED_FREE_SPACE_GB: %.1f, DIR: %s)" % (
		MAX_CACHE_SIZE_GB, REQUIRED_FREE_SPACE_GB,
		 directory
	)
	if not os.path.isdir(directory):
		print "Location %s is not a directory..." % directory
		return
	totalSpace = 0
	freedSpace = 0
	filesInCache = []
	for root, _dirs, files in os.walk(directory):
		for filename in files:
			fLoc = os.path.join(root, filename)
			try:
				statRes = os.stat(fLoc)
			except EnvironmentError:
				pass
			else:
				timeStamp = statRes[stat.ST_ATIME]
				fileSize = statRes[stat.ST_SIZE]
				filesInCache.append( (timeStamp, fLoc, fileSize) )
				totalSpace += fileSize
	#print len(filesInCache), totalSpace
	if MAX_CACHE_SIZE < totalSpace:
		filesInCache.sort(lambda y, x: ( (x[0] - y[0]) > 0)*2 - ((x[0] - y[0])!=0))
	
	# returns number of free space on the disk (in bytes)
	if hasattr(os, "statvfs"):
		freeSpaceFsStat = os.statvfs(directory)
		freeSpace = long(freeSpaceFsStat[statvfs.F_BAVAIL]) * freeSpaceFsStat[statvfs.F_FRSIZE]
	else:
		print "Free space calculation not supported"
		freeSpace = REQUIRED_FREE_SPACE
		
	#print "Free space: %s" % freeSpace
	while (totalSpace > MAX_CACHE_SIZE or freeSpace < REQUIRED_FREE_SPACE) and len(filesInCache) > 0:
		entry = filesInCache.pop()
		try:
			os.remove(entry[1])
			totalSpace -= entry[2]
			freedSpace += entry[2]
			freeSpace += entry[2]
			#print "removed:", entry[0], entry[1], entry[2]
		except:
			pass
	if hasattr(os, "statvfs"):
		# returns number of free space on the disk (in bytes)
		freeSpaceFsStat = os.statvfs(directory)
		freeSpace = long(freeSpaceFsStat[statvfs.F_BAVAIL]) * freeSpaceFsStat[statvfs.F_FRSIZE]
	else:
		freeSpace = 0
	print "Cache size: %.1f GB; Free space: %.1f GB; Removed %.1f MB" % (
		totalSpace / 1.0 / GB, freeSpace / 1.0 / GB, freedSpace / 1.0 / MB)

def main():
	directory = getCacheDir()
	if 'remote/backup' in str(directory) or str(directory).startswith("/remote"):
		print "reduceCacheSize disabled for directory: %s" % directory
	else:
		try:
			cacheCleanup(directory)
		except Exception, ex:
			print ex
			
if __name__ == '__main__':
	main()