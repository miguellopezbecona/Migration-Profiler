read_data_from_dir <- function(path) {
	setwd(path)

	# Files to be read
	file_suffixes <- c("acs", "avg", "min", "max", "alt")
	filenames <- sapply(file_suffixes, function(s) paste(s, ".csv", sep=""))

	fs <- lapply(filenames, function(fn) read.csv(fn, header = TRUE, na.strings = "-1", row.name = 1))

	return (fs)
}


# Bar plot for number of accesses
plot_last_row <- function(dataframe){
	lastRow <- as.numeric(dataframe[nrow(dataframe),])
	barplot(lastRow, names.arg = colnames(dataframe))
}

# Plots generic matrix as a image with log scale
plot_matrix <- function(m, big){
	mi <- min(m, na.rm = TRUE)
	ma <- max(m, na.rm = TRUE)
	max_exp <- floor(log10(ma-mi)) # Nearest flooring exponent to maximum-minimum to scale better
	exps <- 1:max_exp
	vals <- c(mi, mi + 10**exps, ma) # Min and max in scale are the absolute min and max values instead of 0 and big pow or 10 respectively
	num_vals <- length(vals)

	# If the plot is going to be exported as a big image, the graphic will have a bigger width proportion of the image than the legend
	image_prop <- 3
	if(big)
		image_prop <- 25

	# Reverses rows to plot correctly
	m_img <- apply(m, 2, rev)

	colf <- colorRampPalette(c("red", "blue")) # Color gradient
	layout(matrix(1:2,ncol=2), width = c(image_prop,1),height = c(1,1)) # Two figures (image and legend)
	image(1:ncol(m), 1:nrow(m), t(m_img), col = rev(colf(num_vals -1L)), breaks = vals, axes = FALSE) # Plots image
		axis(1, 1:ncol(m), colnames(m))
		axis(2, 1:nrow(m), rev(rownames(m)))
	legend_image <- as.raster(matrix(colf(num_vals), ncol=1)) # Creates legend
	plot(c(0,2),c(0,1),type = 'n', axes = F,xlab = '', ylab = '', main = "Aggregated latency value") #Plots legend
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
	if(filename != "") {
		png(filename, width = 32000, height = 32000/nrow(m), units = 'px', res = 300)
		plot_matrix(m, TRUE)
		dev.off()
	} else
		plot_matrix(m, FALSE)
	
	# Plots last row if wanted
	if(plots_last_row)
		plot_last_row(dataframe)
}

# For "alt" case
plot_alt <- function(df, filename = "") {
	if(filename != "")
		png(filename, width = 6000, height = 4000, units = 'px', res = 300)

	# Plotting page number vs how many different threads accessed to it?
	plot(df$threads_accessed, type="l", col = "red", xaxt = "n", xlab="Page Number", ylab="Number of threads that accessed")
	axis(1, at=1:nrow(df), labels=rownames(df))

	if(filename != "")
		dev.off()
}

data <- read_data_from_dir("~/csvs") # Adjust to the desired folder
#d <- read_data_from_dir("~/data/direct")
#ind <- read_data_from_dir("~/data/indirect")
#int <- read_data_from_dir("~/data/inter")
#all <- read_data_from_dir("~/data/all")

# Let's do some plots for avg file in list
make_plot(data[["avg"]])
#plot_alt(data[["alt"]])

# For testing plots
#m9 <- matrix(c(NA, 0, 100, 500, 1000, 100, 7000, NA, 10), nc=3, nr=3, byrow = TRUE); View(m9); plot_matrix(m9)

