setwd("D:/Shared/pruebasR")

# Reads files for direct and indirect case
#file_suffixes <- c("acs", "avg", "min", "max")
#d_filenames <- sapply(file_suffixes, function(s) paste("d_", s, ".csv", sep=""))
#i_filenames <- sapply(file_suffixes, function(s) paste("i_", s, ".csv", sep=""))
#ds <- lapply(d_filenames, function(fn) read.csv(fn, header = TRUE))
#is <- lapply(i_filenames, function(fn) read.csv(fn, header = TRUE))

# Bar plot for number of accesses
plot_last_row <- function(dataframe){
	lastRow <- as.numeric(dataframe[nrow(dataframe),])
	barplot(lastRow, names.arg = colnames(dataframe))
}

# Plots generic matrix as a image
# TODO: adjust legend size when exporting to big image
plot_matrix <- function(m){
	num_distinct_values <- length(table(m)) # To know how many colours use in gradient
	mi <- min(m, na.rm = TRUE)
	ma <- max(m, na.rm = TRUE)

	# Reverses rows to plot correctly
	m_img <- apply(m, 2, rev)

	colf <- colorRampPalette(c("red", "blue")) # Color gradient
	layout(matrix(1:2,ncol=2), width = c(2,1),height = c(1,1)) # Two figures (image and legend)
	image(1:ncol(m), 1:nrow(m), t(m_img), col = rev(colf(num_distinct_values)), axes = FALSE) # Plots image
		axis(1, 1:ncol(m), colnames(m))
		axis(2, 1:nrow(m), rev(rownames(m)))
	legend_image <- as.raster(matrix(colf(num_distinct_values), ncol=1)) # Creates tegend
	plot(c(0,2),c(0,1),type = 'n', axes = F,xlab = '', ylab = '', main = 'legend title') #Plots legend
	text(x=1.5, y = seq(0,1,l=5), labels = seq(mi,ma,l=5)) # Adds labels to legend
	rasterImage(legend_image, 0, 0, 1, 1)
}

# Makes all the process for a given dataframe: gets last row (used for number of accesses), conversion to matrix, plotting...
make_plot <- function(dataframe, filename = "", max_minus = NA, plots_last_row = FALSE){
	rowsMinusLast <- 1:(nrow(dataframe)-1)
	m <- t(data.matrix(dataframe[rowsMinusLast,])) # Convert to matrix without last row because it is used for number of accesses

	# Turns (max - max_minus) values to NA if wanted
	if(!is.na(max_minus)){
		mx <- max(m, na.rm = "TRUE") # Gets maximum
		cat("Max =", mx, "\n")
		cat("Its position =", which(m == mx, arr.ind = TRUE), "\n")

		filtr <- mx - max_minus
		m[m < filtr] <- NA # Filters
	}
	
	# Saves image plot as png file if filename is not a empty string
	if(filename != "")
		png(filename, width = 32000, height = 32000/nrow(m), units = 'px', res = 300)
	plot_matrix(m)
	if(filename != "")
		dev.off()
	
	# Plots last row if wanted
	if(plots_last_row)
		plot_last_row(dataframe)
}

#node0_cpus <- seq(0,11,2) + 1
#node1_cpus <- seq(1,11,2) + 1

# Reads file
d <- read.csv("max_180.csv", header = TRUE, na.strings = "-1", row.name = 1)

# Some tests
make_plot(d)
make_plot(d, max_minus = 10000)
make_plot(d, plots_last_row = TRUE)

# For testing
#t9 <- matrix(c(NA, 1, 1, 2, NA, 2, 3, NA, NA), nc=3, nr=3, byrow = TRUE); plot_matrix(t9)
