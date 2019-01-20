library(ggplot2)
library(colorspace)


lit_det <- read.csv('literature_det.csv')
lit_det$type <- 'det     '
lit_cd <- read.csv('literature_cd.csv')
lit_cd$type <- 'cd     '
lit_sd <- read.csv('literature_sd.csv')
lit_sd$type <- 'sd     '
lit_nd <- read.csv('literature_nd.csv')
lit_nd$type <- 'nd     '

rand_det <- read.csv('random_det.csv')
rand_det$type <- 'det     '
rand_cd <- read.csv('random_cd.csv')
rand_cd$type <- 'cd     '
rand_sd <- read.csv('random_sd.csv')
rand_sd$type <- 'sd     '
rand_nd <- read.csv('random_nd.csv')
rand_nd$type <- 'nd     '

help("tidyr::separate")
??tidyr::separate

lit <- rbind(lit_det,lit_cd,lit_sd,lit_nd)
rand <- rbind(rand_det,rand_cd,rand_sd,rand_nd)
lit$set <- 'literature formulae'
rand$set <- 'random formulae'
all <- rbind(lit, rand)

all <- tidyr::separate(all, tool, into=c('output','tool','simpl'), sep='\\.')
all$tool <- as.factor(all$tool)
all$output <- as.factor(all$output)

# Reorder the types
all$type <- factor(all$type, levels = c('det     ', 'cd     ', 'sd     ', 'nd     '))

# Keep experiments without simplification
allsimp <- all[all$simpl == 'yes',]
all <- all[all$simpl == 'no',]

levels(all$output)[levels(all$output)=="cd"] <- "cut-deterministic output"
levels(all$output)[levels(all$output)=="sd"] <- "semi-deterministic output"
levels(allsimp$output)[levels(allsimp$output)=="cd"] <- "cut-deterministic output"
levels(allsimp$output)[levels(allsimp$output)=="sd"] <- "semi-deterministic output"


all2 <- dplyr::select(all,formula,tool,states,type,set,output)
all <- tidyr::spread(all2,tool,states)

ggplot(all, aes(y=ltl2ldba,x=seminator,colour=type,shape=type)) +
    geom_abline(slope=1,intercept=0) + # theme_classic() +
    geom_jitter(width = 0.02, height = 0.02, alpha=.8, size=2.5) +
    scale_x_log10(breaks=c(1,10,100,1000)) + scale_y_log10() +
    scale_colour_brewer(palette = "Set1", direction=-1,type = qualitative) +
    scale_shape_manual(values=c(16,17,15,18)) +
    coord_fixed() + facet_grid(output ~ set) +
    theme(legend.margin=margin(2,-2,2,2,"mm"),
          plot.margin=margin(0,0,0,0,"mm"),
          legend.position = c(.5,.41))

ggsave(filename = 'ltl_sem.pdf', width=10, height=8.1)

sd <- all[all$output == 'semi-deterministic output',]

ggplot(sd, aes(y=nba2ldba,x=seminator,colour=type,shape=type)) +
  geom_abline(slope=1,intercept=0) + # theme_classic() +
  geom_jitter(width = 0.02, height = 0.02, alpha=.8, size=2.5) +
  scale_x_log10(breaks=c(1,10,100,1000)) + scale_y_log10(breaks=c(1,10,100,1000)) +
  scale_colour_brewer(palette = "Set1", direction=-1,type = qualitative) +
  scale_shape_manual(values=c(16,17,15,18)) +
  coord_fixed() + facet_grid(output ~ set) +
  theme(legend.margin=margin(2,-2,2,2,"mm"),
        plot.margin=margin(0,0,0,0,"mm"),
        legend.position = c(.5,.505))


ggsave(filename = 'nba_sem.pdf', width=10, height=5.1)

colnames(all)[5] <- "twostep"
ggplot(all, aes(y=twostep,x=seminator,colour=type,shape=type)) +
  geom_abline(slope=1,intercept=0) + # theme_classic() +
  geom_jitter(width = 0.02, height = 0.02, alpha=.8, size=2.5) +
  scale_x_log10(breaks=c(1,10,100,1000)) + scale_y_log10() +
  scale_colour_brewer(palette = "Set1", direction=-1,type = qualitative) +
  scale_shape_manual(values=c(16,17,15,18)) +
  coord_fixed() + facet_grid(output ~ set) +
  theme(legend.margin=margin(2,-2,2,2,"mm"),
        plot.margin=margin(0,0,0,0,"mm"),
        legend.position = c(.5,.41))

ggsave(filename = '2step_sem.pdf', width=10, height=8.1)

### Effect of enter-jump with simpifications
all2s <- dplyr::select(allsimp,formula,tool,states,type,set,output)
allsimp <- tidyr::spread(all2s,tool,states)

ggplot(allsimp, aes(y=sem_enter,x=seminator,colour=type,shape=type)) +
  geom_abline(slope=1,intercept=0) + # theme_classic() +
  geom_jitter(width = 0.02, height = 0.02, alpha=.8, size=2.5) +
  scale_x_log10(breaks=c(1,10,100,1000)) + scale_y_log10() +
  scale_colour_brewer(palette = "Set1", direction=-1,type = qualitative) +
  scale_shape_manual(values=c(16,17,15,18)) +
  coord_fixed() + facet_grid(output ~ set) +
  theme(legend.margin=margin(2,-2,2,2,"mm"),
        plot.margin=margin(0,0,0,0,"mm"),
        legend.position = c(.5,.41))

ggsave(filename = 'enter_sem.pdf', width=10, height=8.1)

ggplot(allsimp, aes(y=sem_enter,x=ltl2ldba,colour=type,shape=type)) +
  geom_abline(slope=1,intercept=0) + # theme_classic() +
  geom_jitter(width = 0.02, height = 0.02, alpha=.8, size=2.5) +
  scale_x_log10(breaks=c(1,10,100,1000)) + scale_y_log10() +
  scale_colour_brewer(palette = "Set1", direction=-1,type = qualitative) +
  scale_shape_manual(values=c(16,17,15,18)) +
  coord_fixed() + facet_grid(output ~ set) +
  theme(legend.margin=margin(2,-2,2,2,"mm"),
        plot.margin=margin(0,0,0,0,"mm"),
        legend.position = c(.5,.41))

ggsave(filename = 'enter_ltl2ldba.pdf', width=10, height=8.1)



#graph(ggplot(lit, aes(x=cd.ltl2ldba.no,y=cd.seminator.no,colour=type,shape=type)))
# ggsave(filename = 'cd_ltl_sem.pdf')
#
#graph(ggplot(lit, aes(x=sd.ltl2ldba.no,y=sd.seminator.no,colour=type,shape=type)))
#ggsave(filename = 'sd_ltl_sem.pdf')

