#setwd("D:/Shared/pruebasR")

# Bar plot for number of accesses
plot_last_row <- function(dataframe){
	lastRow <- as.numeric(dataframe[nrow(dataframe),])
	barplot(lastRow, names.arg = colnames(dataframe))
}

# Plots generic matrix as a image with log scale
# TODO: adjust legend size when exporting to big image
plot_matrix <- function(m){
	mi <- min(m, na.rm = TRUE)
	ma <- max(m, na.rm = TRUE)
	max_exp <- ceiling(log10(ma-mi)) # Nearest exponent to maximum-minimum to scale better
	exps <- 1:max_exp
	vals <- c(mi, mi + 10**exps) # Minimum in scale is the minimum value instead of 0
	num_vals <- length(vals)

	# Reverses rows to plot correctly
	m_img <- apply(m, 2, rev)

	colf <- colorRampPalette(c("red", "blue")) # Color gradient
	layout(matrix(1:2,ncol=2), width = c(2,1),height = c(1,1)) # Two figures (image and legend)
	image(1:ncol(m), 1:nrow(m), t(m_img), col = rev(colf(num_vals -1L)), breaks = vals, axes = FALSE) # Plots image
		axis(1, 1:ncol(m), colnames(m))
		axis(2, 1:nrow(m), rev(rownames(m)))
	legend_image <- as.raster(matrix(colf(num_vals), ncol=1)) # Creates legend
	plot(c(0,2),c(0,1),type = 'n', axes = F,xlab = '', ylab = '', main = 'legend title') #Plots legend
	text(x=1.5, y = seq(0,1,l=num_vals), labels = vals) # Adds labels to legend
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

# Prepares files to be read
file_suffixes <- c("acs", "avg", "min", "max")
filenames <- sapply(file_suffixes, function(s) paste(s, ".csv", sep=""))

# Reads files for direct and indirect case
d_filenames <- sapply(file_suffixes, function(s) paste("d_", s, ".csv", sep=""))
i_filenames <- sapply(file_suffixes, function(s) paste("i_", s, ".csv", sep=""))
#ds <- lapply(d_filenames, function(fn) read.csv(fn, header = TRUE, na.strings = "-1", row.name = 1))
#is <- lapply(i_filenames, function(fn) read.csv(fn, header = TRUE, na.strings = "-1", row.name = 1))

# Or just ones without prefixes
fs <- lapply(filenames, function(fn) read.csv(fn, header = TRUE, na.strings = "-1", row.name = 1))

# Let's do some plots for avg file in list
l <- fs # ds # is
make_plot(l[["avg"]])
make_plot(l[["avg"]], max_minus = 10000)
make_plot(l[["avg"]], plots_last_row = TRUE)

# For testing plots
#m9 <- matrix(c(NA, 0, 100, 500, 1000, 100, 7000, NA, 10), nc=3, nr=3, byrow = TRUE); View(m9); plot_matrix(m9)
