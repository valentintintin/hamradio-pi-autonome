﻿// <auto-generated />
using System;
using Microsoft.EntityFrameworkCore;
using Microsoft.EntityFrameworkCore.Infrastructure;
using Microsoft.EntityFrameworkCore.Migrations;
using Microsoft.EntityFrameworkCore.Storage.ValueConversion;
using Monitor.Context;

#nullable disable

namespace Monitor.Migrations
{
    [DbContext(typeof(DataContext))]
    [Migration("20230519165529_Initial")]
    partial class Initial
    {
        /// <inheritdoc />
        protected override void BuildTargetModel(ModelBuilder modelBuilder)
        {
#pragma warning disable 612, 618
            modelBuilder.HasAnnotation("ProductVersion", "7.0.5");

            modelBuilder.Entity("Monitor.Context.Entities.Mppt", b =>
                {
                    b.Property<long>("Id")
                        .ValueGeneratedOnAdd()
                        .HasColumnType("INTEGER");

                    b.Property<bool>("Alert")
                        .HasColumnType("INTEGER");

                    b.Property<int>("BatteryCurrent")
                        .HasColumnType("INTEGER");

                    b.Property<int>("BatteryVoltage")
                        .HasColumnType("INTEGER");

                    b.Property<DateTime>("CreatedAt")
                        .HasColumnType("TEXT");

                    b.Property<int>("CurrentCharge")
                        .HasColumnType("INTEGER");

                    b.Property<bool>("Night")
                        .HasColumnType("INTEGER");

                    b.Property<int>("SolarCurrent")
                        .HasColumnType("INTEGER");

                    b.Property<int>("SolarVoltage")
                        .HasColumnType("INTEGER");

                    b.Property<int>("Status")
                        .HasColumnType("INTEGER");

                    b.Property<bool>("WatchdogEnabled")
                        .HasColumnType("INTEGER");

                    b.Property<long>("WatchdogPowerOffTime")
                        .HasColumnType("INTEGER");

                    b.HasKey("Id");

                    b.ToTable("Mppts");
                });

            modelBuilder.Entity("Monitor.Context.Entities.Weather", b =>
                {
                    b.Property<long>("Id")
                        .ValueGeneratedOnAdd()
                        .HasColumnType("INTEGER");

                    b.Property<DateTime>("CreatedAt")
                        .HasColumnType("TEXT");

                    b.Property<int>("Humidity")
                        .HasColumnType("INTEGER");

                    b.Property<double>("Temperature")
                        .HasColumnType("REAL");

                    b.HasKey("Id");

                    b.ToTable("Weathers");
                });
#pragma warning restore 612, 618
        }
    }
}
