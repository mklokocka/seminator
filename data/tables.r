library(tables)
booktabs()

lit_det <- read.csv('literature_det.csv')
lit_det$type <- as.factor('\\pin{1}~det')
lit_cd <- read.csv('literature_cd.csv')
lit_cd$type <- as.factor('\\pin{2}~cd')
lit_sd <- read.csv('literature_sd.csv')
lit_sd$type <- as.factor('\\pin{3}~sd')
lit_nd <- read.csv('literature_nd.csv',)
lit_nd$type <- as.factor('\\pin{4}~nd')
rand_det <- read.csv('random_det.csv')
rand_det$type <- as.factor('\\pin{1}~det')
rand_cd <- read.csv('random_cd.csv')
rand_cd$type <- as.factor('\\pin{2}~cd')
rand_sd <- read.csv('random_sd.csv')
rand_sd$type <- as.factor('\\pin{3}~sd')
rand_nd <- read.csv('random_nd.csv')
rand_nd$type <- as.factor('\\pin{4}~nd')

lit <- rbind(lit_det,lit_cd,lit_sd,lit_nd)
rand <- rbind(rand_det,rand_cd,rand_sd,rand_nd)
lit <- dplyr::select(lit,formula,tool,states,type)
rand <- dplyr::select(rand,formula,tool,states,type)

## Extract the problematic fomrula
lit2 <- tidyr::spread(dplyr::select(lit,formula,tool,states,type),tool,states)
fail <- lit2[!complete.cases(lit2),] 
lit2 <- lit2[complete.cases(lit2),] 
lit <- tidyr::gather(lit2,'tool','states',3:16)
fail <- tidyr::gather(fail,'tool','states',3:16)

lit<-tidyr::separate(lit, tool,into = c("requested", "tool", "red"))
rand<-tidyr::separate(rand, tool,into = c("requested", "tool", "red"))
fail<-tidyr::separate(fail, tool,into = c("requested", "tool", "red"))

lit$source <- as.factor('literature')
rand$source <- as.factor('random')
merged <- rbind(rand,lit)

merged$tool[merged$tool=="seminator"] <- "\\Seminator"
merged$tool[merged$tool=="ltl2ldba"] <- "\\ltl"
merged$tool[merged$tool=="nba2ldba"] <- "\\nba"
merged$tool[merged$tool=="cy"] <- "CY"


fail$tool[fail$tool=="seminator"] <- "\\Seminator"
fail$tool[fail$tool=="ltl2ldba"] <- "\\ltl"
fail$tool[fail$tool=="nba2ldba"] <- "\\nba"
fail$tool[fail$tool=="cy"] <- "CY"
  
merged_cd <- merged[merged$requested == 'cd',]
merged_sd <- merged[merged$requested == 'sd',]
fail_sd <- fail[fail$requested == 'sd',]
fail_cd <- fail[fail$requested == 'cd',]
fail_sd$source <- "T/O (lit.)"
fail_cd$source <- "literature"
merged_cd <- rbind(merged_cd,fail_cd)

mysum <- function(x) {
    res <- (sum(x,na.rm = TRUE))
    if (res == 0) {
        return ("---")
    }
    return (res)
}

formula <- (Heading() * RowFactor(source,texify = FALSE,spacing=1,space=.5) * Factor(type,texify=FALSE))~ 
    (Heading() * (tool=='\\Seminator')) * (Heading() * (red=='yes') * Heading('n') * 1) + 
    (Heading() * Factor(tool,texify=FALSE) * Heading() * Factor(red) * Heading() * states * 
                 Heading() * mysum)

tab_cd <- tabular(formula,merged_cd)
latex(tab_cd,file='tab_cd.tex',booktabs = TRUE)

tab_f <- tabular(formula,data=rbind(merged_sd,fail_sd))
tab_f
tab<-tab_f[tab_f[,1] > 0]
latex(tab,file='tab_sd.tex',booktabs = TRUE)
