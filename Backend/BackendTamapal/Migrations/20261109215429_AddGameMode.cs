using Microsoft.EntityFrameworkCore.Migrations;

#nullable disable

namespace BackendTamapal.Migrations
{
    /// <inheritdoc />
    public partial class AddGameMode : Migration
    {
        /// <inheritdoc />
        protected override void Up(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.AddColumn<string>(
                name: "Mode",
                table: "GameResults",
                type: "text",
                nullable: false,
                defaultValue: "solo"); // gamle rækker (og den gamle server-version) er solo
        }

        /// <inheritdoc />
        protected override void Down(MigrationBuilder migrationBuilder)
        {
            migrationBuilder.DropColumn(
                name: "Mode",
                table: "GameResults");
        }
    }
}
