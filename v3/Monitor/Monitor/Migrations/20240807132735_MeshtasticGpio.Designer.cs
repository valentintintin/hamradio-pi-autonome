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
    [Migration("20240807132735_MeshtasticGpio")]
    partial class MeshtasticGpio
    {
        /// <inheritdoc />
        protected override void BuildTargetModel(ModelBuilder modelBuilder)
        {
#pragma warning disable 612, 618
            modelBuilder.HasAnnotation("ProductVersion", "8.0.7");

            modelBuilder.Entity("Monitor.Context.Entities.BbsMessage", b =>
                {
                    b.Property<long>("Id")
                        .ValueGeneratedOnAdd()
                        .HasColumnType("INTEGER");

                    b.Property<DateTime>("CreatedAt")
                        .HasColumnType("TEXT");

                    b.Property<string>("From")
                        .IsRequired()
                        .HasMaxLength(16)
                        .HasColumnType("TEXT");

                    b.Property<string>("Message")
                        .IsRequired()
                        .HasMaxLength(200)
                        .HasColumnType("TEXT");

                    b.Property<DateTime?>("ReadAt")
                        .HasColumnType("TEXT");

                    b.Property<DateTime?>("RemindedAt")
                        .HasColumnType("TEXT");

                    b.Property<string>("To")
                        .IsRequired()
                        .HasMaxLength(16)
                        .HasColumnType("TEXT");

                    b.HasKey("Id");

                    b.ToTable("BbsMessages");
                });

            modelBuilder.Entity("Monitor.Context.Entities.Config", b =>
                {
                    b.Property<long>("Id")
                        .ValueGeneratedOnAdd()
                        .HasColumnType("INTEGER");

                    b.Property<DateTime>("CreatedAt")
                        .HasColumnType("TEXT");

                    b.Property<string>("Name")
                        .IsRequired()
                        .HasMaxLength(64)
                        .HasColumnType("TEXT");

                    b.Property<string>("Value")
                        .IsRequired()
                        .HasMaxLength(128)
                        .HasColumnType("TEXT");

                    b.HasKey("Id");

                    b.ToTable("Configs");
                });

            modelBuilder.Entity("Monitor.Context.Entities.LoRa", b =>
                {
                    b.Property<long>("Id")
                        .ValueGeneratedOnAdd()
                        .HasColumnType("INTEGER");

                    b.Property<DateTime>("CreatedAt")
                        .HasColumnType("TEXT");

                    b.Property<string>("Frame")
                        .IsRequired()
                        .HasMaxLength(512)
                        .HasColumnType("TEXT");

                    b.Property<bool>("IsMeshtastic")
                        .HasColumnType("INTEGER");

                    b.Property<bool>("IsTx")
                        .HasColumnType("INTEGER");

                    b.Property<string>("Sender")
                        .HasMaxLength(64)
                        .HasColumnType("TEXT");

                    b.HasKey("Id");

                    b.ToTable("LoRas");
                });

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

                    b.Property<string>("StatusString")
                        .HasColumnType("TEXT");

                    b.Property<float>("Temperature")
                        .HasColumnType("REAL");

                    b.Property<bool>("WatchdogEnabled")
                        .HasColumnType("INTEGER");

                    b.Property<long>("WatchdogPowerOffTime")
                        .HasColumnType("INTEGER");

                    b.HasKey("Id");

                    b.ToTable("Mppts");
                });

            modelBuilder.Entity("Monitor.Context.Entities.System", b =>
                {
                    b.Property<long>("Id")
                        .ValueGeneratedOnAdd()
                        .HasColumnType("INTEGER");

                    b.Property<bool>("BoxOpened")
                        .HasColumnType("INTEGER");

                    b.Property<DateTime>("CreatedAt")
                        .HasColumnType("TEXT");

                    b.Property<long>("McuUptime")
                        .HasColumnType("INTEGER");

                    b.Property<bool>("Meshtastic")
                        .HasColumnType("INTEGER");

                    b.Property<int>("NbError")
                        .HasColumnType("INTEGER");

                    b.Property<bool>("Npr")
                        .HasColumnType("INTEGER");

                    b.Property<float>("TemperatureRtc")
                        .HasColumnType("REAL");

                    b.Property<long>("Uptime")
                        .HasColumnType("INTEGER");

                    b.Property<bool>("Wifi")
                        .HasColumnType("INTEGER");

                    b.HasKey("Id");

                    b.ToTable("Systems");
                });

            modelBuilder.Entity("Monitor.Context.Entities.Weather", b =>
                {
                    b.Property<long>("Id")
                        .ValueGeneratedOnAdd()
                        .HasColumnType("INTEGER");

                    b.Property<DateTime>("CreatedAt")
                        .HasColumnType("TEXT");

                    b.Property<float>("Humidity")
                        .HasColumnType("REAL");

                    b.Property<float>("Pressure")
                        .HasColumnType("REAL");

                    b.Property<float>("Temperature")
                        .HasColumnType("REAL");

                    b.HasKey("Id");

                    b.ToTable("Weathers");
                });
#pragma warning restore 612, 618
        }
    }
}